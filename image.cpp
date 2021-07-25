#include <iostream>
#include <fstream>
#include "image.h"
#include <windows.h>
#include <zlib.h>

using namespace std;

READER::READER() {
	data = std::vector<unsigned char>();
	length = 0;
	pos = 0;
}

READER::READER(std::string path) {
	ifstream file;
	file.open(path, ios::in | ios::binary);
	file.unsetf(std::ios::skipws);

	file.seekg(0, std::ios::end);
	std::streampos size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	file.read(buffer.data(), size);
	file.close();
	std::vector<unsigned char> newbuf(size);
	for (int i = 0; i < size; i++) {
		newbuf[i] = buffer[i];
	}
	data = newbuf;
	length = data.size();
	pos = 0;
}

READER::READER(std::vector<unsigned char> adata) {
	data = adata;
	length = data.size();
	pos = 0;
}

unsigned int READER::ReadRU8() {
	if (pos > 0) {
		return data.at(pos - 1);
	}
	else {
		return 0;
	}
}

unsigned int READER::ReadU8() {
	return pos < length ? data.at(pos++) : 0;
}

unsigned int READER::ReadU16() {
	char val1 = ReadU8();
	char val2 = ReadU8();
	return (val1 << 8) | val2;
}

unsigned int READER::ReadU24() {
	char val1 = ReadU8();
	char val2 = ReadU8();
	char val3 = ReadU8();
	return (val1 << 16) | (val2 << 8) | val3;
}

unsigned int READER::ReadU32() {
	unsigned char val1 = ReadU8();
	unsigned char val2 = ReadU8();
	unsigned char val3 = ReadU8();
	unsigned char val4 = ReadU8();
	return (val1 << 24) | (val2 << 16) | (val3 << 8) | val4;
}

int READER::ReadS8() {
	return (int)ReadU8();
}

int READER::ReadS16() {
	return (int)ReadU16();
}

int READER::ReadS24() {
	return (int)ReadU24();
}

int READER::ReadS32() {
	return (int)ReadU32();
}

void READER::SeekPos(int apos) {
	pos = apos;
}

std::vector<unsigned char> READER::ReadBytes(unsigned int length) {
	vector<unsigned char> buffer(length);
	for (unsigned int i = 0; i < length; i++) {
		buffer[i] = ReadU8();
	}
	return buffer;
}

std::string READER::ReadString(unsigned int length) {
	vector<unsigned char> buffer = ReadBytes(length);
	return string(buffer.begin(), buffer.end());
}

IMAGE::IMAGE() {
	width = 0;
	height = 0;
	data = std::vector<unsigned int>();
}

PNG::PNG() {
	reader = READER();
	chunks = std::map<unsigned int, std::vector<unsigned char>>();
	idat = std::vector<std::vector<unsigned char>>();
}

PNG::PNG(READER stream) {
	reader = stream;
	chunks = std::map<unsigned int, std::vector<unsigned char>>();
	idat = std::vector<std::vector<unsigned char>>();
}

PNG::PNG(std::string path) {
	reader = READER(path);
	chunks = std::map<unsigned int, std::vector<unsigned char>>();
	idat = std::vector<std::vector<unsigned char>>();
}

const char pngSig[8]{ 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

void PNG::ReadChunks() {
	vector<unsigned char> magic = reader.ReadBytes(8);
	if (memcmp(magic.data(), pngSig, 8) == 0) {
		while (reader.pos < reader.length) {
			unsigned int chunkSize = reader.ReadU32();
			unsigned int chunkType = reader.ReadU32();
			vector<unsigned char> chunkData(0);
			if (chunkSize > 0) {
				chunkData = reader.ReadBytes(chunkSize);
			}
			unsigned int chunkCRC = reader.ReadU32();

			switch (chunkType) {
			default:
				chunks[chunkType] = chunkData;
				break;
			case(0x49444154):
				idat.push_back(chunkData);
				break;
			case(0x49454E44):
				return;
			}
		}
	}
}

unsigned int DoPaeth(unsigned int a, unsigned int b, unsigned int c) {
	int p = (int)a + (int)b - (int)c;
	int pa = abs(p - (int)a);
	int pb = abs(p - (int)b);
	int pc = abs(p - (int)c);
	unsigned int pr = 0;
	if (pa <= pb && pa <= pc) {
		pr = a;
	}
	else if (pb <= pc) {
		pr = b;
	}
	else {
		pr = c;
	}
	return pr;
}

//return data in RGBA32
IMAGE PNG::Process() {
	IMAGE newImg = IMAGE();

	READER IHDR = READER(chunks[0x49484452]);
	unsigned int width = IHDR.ReadU32();
	unsigned int height = IHDR.ReadU32();
	unsigned char bitdepth = IHDR.ReadU8();
	unsigned char colortype = IHDR.ReadU8();
	unsigned char comp = IHDR.ReadU8();
	unsigned char filter = IHDR.ReadU8();
	unsigned char interlace = IHDR.ReadU8();

	//deflate compression, no filter, no interlace
	if (comp == 0 && filter == 0 && interlace == 0) {
		//write all idat chunks to single vector
		size_t datcount = idat.size();
		unsigned int datsize = 0;

		//get datasize
		for (unsigned int i = 0; i < datcount; i++) {
			datsize += idat[i].size();
		}

		vector<unsigned char> PictureData(datsize);
		unsigned int counter = 0;

		//copy data to combined chunk
		for (unsigned int i = 0; i < datcount; i++) {
			vector<unsigned char> dchunk = idat[i];
			for (unsigned int a = 0; a < dchunk.size(); a++) {
				PictureData[counter++] = dchunk[a];
			}
		}

		//inflate stream
		vector<vector<unsigned char>> inflated;

		vector<unsigned char> infvc(8192);

		z_stream InflateStream;
		InflateStream.zalloc = Z_NULL;
		InflateStream.zfree = Z_NULL;
		InflateStream.opaque = Z_NULL;
		InflateStream.avail_in = PictureData.size();
		InflateStream.next_in = (Bytef*)PictureData.data();

		inflateInit(&InflateStream);

		while (true) {
			InflateStream.avail_out = 8192;
			InflateStream.next_out = (Bytef*)infvc.data();
			int result = inflate(&InflateStream, Z_NO_FLUSH);
			//OutputDebugStringA((to_string(result) + "\n").c_str());
			if (result == Z_STREAM_END || result == Z_OK) {
				vector<unsigned char> infvcd(8192);
				memcpy(infvcd.data(), infvc.data(), 8192);
				inflated.push_back(infvcd);
			}
			if (result == Z_STREAM_END) {
				break;
			}
		}

		inflateEnd(&InflateStream);

		//write all inflated chunks to single vector
		size_t outcount = inflated.size();
		unsigned int outsize = 0;

		//get datasize
		for (unsigned int i = 0; i < outcount; i++) {
			outsize += inflated[i].size();
		}

		vector<unsigned char> PixelData(outsize);
		unsigned int pcounter = 0;

		//copy data to combined chunk
		for (unsigned int i = 0; i < outcount; i++) {
			vector<unsigned char> dchunk = inflated[i];
			for (int a = 0; a < dchunk.size(); a++) {
				PixelData[pcounter++] = dchunk[a];
			}
		}

		READER PixelReader = READER(PixelData);
		std::vector<unsigned int> FinalData(width * height);

		//process pixel data
		switch (colortype) {
		case 2: //Truecolor
			switch (bitdepth) {
			case 8: //8 bit RGB
				std::vector<unsigned int> RedChannel(width * height);
				std::vector<unsigned int> GreenChannel(width * height);
				std::vector<unsigned int> BlueChannel(width * height);

				for (unsigned int y = 0; y < height; y++) {
					unsigned int filterType = PixelReader.ReadU8();

					for (unsigned int x = 0; x < width; x++) {
						unsigned int red = PixelReader.ReadU8();
						unsigned int green = PixelReader.ReadU8();
						unsigned int blue = PixelReader.ReadU8();
						unsigned int loc = y * width + x;

						unsigned int redA = 0;
						unsigned int greenA = 0;
						unsigned int blueA = 0;
						unsigned int redB = 0;
						unsigned int greenB = 0;
						unsigned int blueB = 0;
						unsigned int redC = 0;
						unsigned int greenC = 0;
						unsigned int blueC = 0;

						switch (filterType) {
						case 1: //last pixel
							if (x > 0) {
								redA = RedChannel[loc - 1];
								greenA = GreenChannel[loc - 1];
								blueA = BlueChannel[loc - 1];
							}
							red = (red + redA) % 256;
							green = (green + greenA) % 256;
							blue = (blue + blueA) % 256;
							break;
						case 2: //last scanline
							if (y > 0) {
								redB = RedChannel[loc - width];
								greenB = GreenChannel[loc - width];
								blueB = BlueChannel[loc - width];
							}
							red = (red + redB) % 256;
							green = (green + greenB) % 256;
							blue = (blue + blueB) % 256;
							break;
						case 3:
							if (x > 0) {
								redA = RedChannel[loc - 1];
								greenA = GreenChannel[loc - 1];
								blueA = BlueChannel[loc - 1];
							}
							if (y > 0) {
								redB = RedChannel[loc - width];
								greenB = GreenChannel[loc - width];
								blueB = BlueChannel[loc - width];
							}
							red = (red + ((redA + redB) / (unsigned int)2)) % 256;
							green = (green + ((greenA + greenB) / (unsigned int)2)) % 256;
							blue = (blue + ((blueA + blueB) / (unsigned int)2)) % 256;
							break;
						case 4:
							if (x > 0) {
								redA = RedChannel[loc - 1];
								greenA = GreenChannel[loc - 1];
								blueA = BlueChannel[loc - 1];
							}
							if (y > 0) {
								redB = RedChannel[loc - width];
								greenB = GreenChannel[loc - width];
								blueB = BlueChannel[loc - width];
							}
							if (x > 0 && y > 0) {
								redC = RedChannel[loc - width - 1];
								greenC = GreenChannel[loc - width - 1];
								blueC = BlueChannel[loc - width - 1];
							}

							red = (red + DoPaeth(redA, redB, redC)) % 256;
							green = (green + DoPaeth(greenA, greenB, greenC)) % 256;
							blue = (blue + DoPaeth(blueA, blueB, blueC)) % 256;
							break;
						}

						RedChannel[loc] = red;
						GreenChannel[loc] = green;
						BlueChannel[loc] = blue;
					}
				}

				for (unsigned int i = 0; i < width * height; i++) {
					FinalData[i] = (RedChannel[i] << 24) | (GreenChannel[i] << 16) | (BlueChannel[i] << 8) | 0xFF;
				}

				newImg.width = width;
				newImg.height = height;
				newImg.data = FinalData;
				break;
			}
			break;
		case 6: //Truecolor with alpha channel
			switch (bitdepth) {
			case 8: //8 bit RGBA
				std::vector<unsigned int> RedChannel(width * height);
				std::vector<unsigned int> GreenChannel(width * height);
				std::vector<unsigned int> BlueChannel(width * height);
				std::vector<unsigned int> AlphaChannel(width * height);

				for (unsigned int y = 0; y < height; y++) {
					unsigned int filterType = PixelReader.ReadU8();

					for (unsigned int x = 0; x < width; x++) {
						unsigned int red = PixelReader.ReadU8();
						unsigned int green = PixelReader.ReadU8();
						unsigned int blue = PixelReader.ReadU8();
						unsigned int alpha = PixelReader.ReadU8();
						unsigned int loc = y * width + x;

						unsigned int redA = 0;
						unsigned int greenA = 0;
						unsigned int blueA = 0;
						unsigned int alphaA = 0;
						unsigned int redB = 0;
						unsigned int greenB = 0;
						unsigned int blueB = 0;
						unsigned int alphaB = 0;
						unsigned int redC = 0;
						unsigned int greenC = 0;
						unsigned int blueC = 0;
						unsigned int alphaC = 0;

						switch (filterType) {
						case 1: //last pixel
							if (x > 0) {
								redA = RedChannel[loc - 1];
								greenA = GreenChannel[loc - 1];
								blueA = BlueChannel[loc - 1];
								alphaA = AlphaChannel[loc - 1];
							}
							red = (red + redA) % 256;
							green = (green + greenA) % 256;
							blue = (blue + blueA) % 256;
							alpha = (alpha + alphaA) % 256;
							break;
						case 2: //last scanline
							if (y > 0) {
								redB = RedChannel[loc - width];
								greenB = GreenChannel[loc - width];
								blueB = BlueChannel[loc - width];
								alphaB = AlphaChannel[loc - width];
							}
							red = (red + redB) % 256;
							green = (green + greenB) % 256;
							blue = (blue + blueB) % 256;
							alpha = (alpha + alphaB) % 256;
							break;
						case 3:
							if (x > 0) {
								redA = RedChannel[loc - 1];
								greenA = GreenChannel[loc - 1];
								blueA = BlueChannel[loc - 1];
								alphaA = AlphaChannel[loc - 1];
							}
							if (y > 0) {
								redB = RedChannel[loc - width];
								greenB = GreenChannel[loc - width];
								blueB = BlueChannel[loc - width];
								alphaB = AlphaChannel[loc - width];
							}
							red = (red + ((redA + redB) / (unsigned int)2)) % 256;
							green = (green + ((greenA + greenB) / (unsigned int)2)) % 256;
							blue = (blue + ((blueA + blueB) / (unsigned int)2)) % 256;
							alpha = (alpha + ((alphaA + alphaB) / (unsigned int)2)) % 256;
							break;
						case 4:
							if (x > 0) {
								redA = RedChannel[loc - 1];
								greenA = GreenChannel[loc - 1];
								blueA = BlueChannel[loc - 1];
								alphaA = AlphaChannel[loc - 1];
							}
							if (y > 0) {
								redB = RedChannel[loc - width];
								greenB = GreenChannel[loc - width];
								blueB = BlueChannel[loc - width];
								alphaB = AlphaChannel[loc - width];
							}
							if (x > 0 && y > 0) {
								redC = RedChannel[loc - width - 1];
								greenC = GreenChannel[loc - width - 1];
								blueC = BlueChannel[loc - width - 1];
								alphaC = AlphaChannel[loc - width - 1];
							}

							red = (red + DoPaeth(redA, redB, redC)) % 256;
							green = (green + DoPaeth(greenA, greenB, greenC)) % 256;
							blue = (blue + DoPaeth(blueA, blueB, blueC)) % 256;
							alpha = (alpha + DoPaeth(alphaA, alphaB, alphaC)) % 256;
							break;
						}

						RedChannel[loc] = red;
						GreenChannel[loc] = green;
						BlueChannel[loc] = blue;
						AlphaChannel[loc] = alpha;
					}
				}

				for (unsigned int i = 0; i < width * height; i++) {
					FinalData[i] = (RedChannel[i] << 24) | (GreenChannel[i] << 16) | (BlueChannel[i] << 8) | AlphaChannel[i];
				}

				newImg.width = width;
				newImg.height = height;
				newImg.data = FinalData;
				break;
			}
			break;
		}
	}

	return newImg;
}