#pragma once
#include <string>
#include <vector>
#include <map>

class READER {
public:
	READER();
	READER(std::string path);
	READER(std::vector<unsigned char> adata);
	void SeekPos(int apos);
	int ReadS8();
	int ReadS16();
	int ReadS24();
	int ReadS32();
	unsigned int ReadRU8();
	unsigned int ReadU8();
	unsigned int ReadU16();
	unsigned int ReadU24();
	unsigned int ReadU32();
	std::vector<unsigned char> ReadBytes(unsigned int length);
	std::string ReadString(unsigned int length);
	unsigned int pos;
	unsigned int length;
private:
	std::vector<unsigned char> data;
};

class IMAGE {
public:
	IMAGE();
	IMAGE(unsigned int awidth, unsigned int aheight, std::vector<unsigned int> adata) { width = awidth; height = aheight; data = adata; }
	unsigned int width;
	unsigned int height;
	std::vector<unsigned int> data;
};

class PNG {
public:
	PNG();
	PNG(READER stream);
	PNG(std::string path);
	void ReadChunks();
	IMAGE Process();
private:
	READER reader;
	std::map<unsigned int, std::vector<unsigned char>> chunks;
	std::vector<std::vector<unsigned char>> idat;
};