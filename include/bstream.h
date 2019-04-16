#pragma once
#include <iostream>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <cassert>
#include <algorithm>

namespace bStream {

uint32_t swap32(uint32_t v);
uint16_t swap16(uint16_t v);

template <typename T>
static inline const T * OffsetPointer(const void * ptr, size_t offs) {
  uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
  p += offs;
  return reinterpret_cast<const T *>(p);
}

template < typename T >
static inline T * OffsetWritePointer(void * ptr, size_t offs) {
  uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
  p += offs;
  return reinterpret_cast<T *>(p);
}

enum OpenMode {
	In,
	Out
};

enum Endianess{
	Big, Little
};

Endianess getSystemEndianess();

class CStream {
	public:
		virtual uint8_t readUInt8() = 0;
		virtual uint16_t readUInt16() = 0;
		virtual uint32_t readUInt32() = 0;
};

class CFileStream : public CStream {
private:
	std::fstream base;
	std::string filePath;
	OpenMode mode;
	Endianess order;
	Endianess systemOrder;

public:

	template<typename T>
	T readStruct(){
		assert(mode == OpenMode::In);
		T out;
		base.read((char*)&out, sizeof(T));
		return out;
	}

	//read functions
	int8_t readInt8();
	uint8_t readUInt8();
	int16_t readInt16();
	uint16_t readUInt16();
	int32_t readInt32();
	uint32_t readUInt32();
	float readFloat();
	char* readBytes(size_t);
	std::string readWString(size_t);
	std::string readString(size_t);

	//write functions
	void writeInt8(int8_t);
	void writeUInt8(uint8_t);
	void writeInt16(int16_t);
	void writeUInt16(uint16_t);
	void writeInt32(int32_t);
	void writeUInt32(uint32_t);
	void writeFloat(float);
	void writeBytes(char*, size_t);
	void writeString(std::string);

	//utility functions
	size_t getSize();
	long tell();
	void seek(long);
	std::string getPath();

	uint8_t peekU8(int);
	uint16_t peekU16(int);
	uint32_t peekU32(int);

	int8_t peekI8(int);
	int16_t peekI16(int);
	int32_t peekI32(int);

	std::fstream &getStream();

	CFileStream(std::string, Endianess, OpenMode mod = OpenMode::In);
	CFileStream(std::string, OpenMode mod = OpenMode::In);
	CFileStream() {}
	~CFileStream() {this->base.close();}
};

class CMemoryStream : public CStream {
	private:
		uint8_t* mBuffer;
		size_t mPosition;
		size_t mSize;
		size_t mCapacity;
		int8_t mHasInternalBuffer;

		OpenMode mOpenMode;
		Endianess order;
		Endianess systemOrder;

		bool Reserve(size_t);
	
	public:

		size_t getSize();
		size_t getCapacity();
		
		int8_t readInt8();
		uint8_t readUInt8();

		int16_t readInt16();
		uint16_t readUInt16();

		int32_t readInt32();
		uint32_t readUInt32();

		void writeInt8(int8_t);
		void writeUInt8(uint8_t);
		
		void writeInt16(int16_t);
		void writeUInt16(uint16_t);

		void writeInt32(int32_t);
		void writeUInt32(uint32_t);

		void writeFloat(float);
		void writeBytes(char*, size_t);
		void writeString(std::string);

		std::string readString(size_t);

		void seek(size_t pos);

		uint8_t* getBuffer();

		CMemoryStream(uint8_t*, size_t, Endianess, OpenMode);
		CMemoryStream(size_t, Endianess, OpenMode);
		CMemoryStream(){}
		~CMemoryStream(){
			if(mHasInternalBuffer){
				delete[] mBuffer;
			}
		}

};
}

#if defined(BSTREAM_IMPLEMENTATION)
namespace bStream {

uint32_t swap32(uint32_t r){
	return ( ((r>>24)&0xFF) | ((r<<8) & 0xFF0000) | ((r>>8)&0xFF00) | ((r<<24)&0xFF000000));
}

uint16_t swap16(uint16_t r){
	return ( ((r<<8)&0xFF00) | ((r>>8)&0x00FF) );
}

Endianess getSystemEndianess(){
	union {
		uint32_t integer;
		uint8_t bytes[sizeof(uint32_t)];
	} check;
	check.integer = 0x01020304U;
	return (check.bytes[0] == 0x01 ? Endianess::Big : Endianess::Little);
}

CFileStream::CFileStream(std::string path, Endianess ord, OpenMode mod){
	base.open(path, (mod == OpenMode::In ? std::ios::in : std::ios::out) | std::ios::binary);
	filePath = path;
	order = ord;
	mode = mod;
	systemOrder = getSystemEndianess();
}

CFileStream::CFileStream(std::string path, OpenMode mod){
	base.open(path, (mod == OpenMode::In ? std::ios::in : std::ios::out) | std::ios::binary);
	filePath = path;
	mode = mod;
	systemOrder = getSystemEndianess();
	order = getSystemEndianess();
}

std::fstream &CFileStream::getStream(){
	return base;
}

std::string CFileStream::getPath(){
	return filePath;
}


void CFileStream::seek(long pos){
	base.seekg(pos, base.beg);
}

long CFileStream::tell(){
	return base.tellg();
}

uint32_t CFileStream::readUInt32(){
	assert(mode == OpenMode::In);
	uint32_t r;
	base.read((char*)&r, sizeof(uint32_t));
	if(order != systemOrder){
		return swap32(r);
	}
	else{
		return r;
	}
}

int32_t CFileStream::readInt32(){
	assert(mode == OpenMode::In);
	int32_t r;
	base.read((char*)&r, sizeof(int32_t));
	if(order != systemOrder){
		return swap32(r);
	}
	else{
		return r;
	}
}

uint16_t CFileStream::readUInt16(){
	assert(mode == OpenMode::In);
	uint16_t r;
	base.read((char*)&r, sizeof(uint16_t));
	if(order != systemOrder){
		return swap16(r);
	}
	else{
		return r;
	}
}

int16_t CFileStream::readInt16(){
	assert(mode == OpenMode::In);
	int16_t r;
	base.read((char*)&r, sizeof(int16_t));
	if(order != systemOrder){
		return swap16(r);
	}
	else{
		return r;
	}
}

uint8_t CFileStream::readUInt8(){
	assert(mode == OpenMode::In);
	uint8_t r;
	base.read((char*)&r, sizeof(uint8_t));
	return r;
}

int8_t CFileStream::readInt8(){
	assert(mode == OpenMode::In);
	int8_t r;
	base.read((char*)&r, sizeof(int8_t));
	return r;
}

float CFileStream::readFloat(){
	assert(mode == OpenMode::In);
	char buff[sizeof(float)];
	base.read(buff, sizeof(float));
	if(order != systemOrder){
		char temp[sizeof(float)];
		temp[0] = buff[3];
		temp[1] = buff[2];
		temp[2] = buff[1];
		temp[3] = buff[0];
		return *((float*)temp);
	}
	return *((float*)buff);
}

char* CFileStream::readBytes(size_t size){
	assert(mode == OpenMode::In);
	char* buffer = new char[size];
	base.read(buffer, size);
	return buffer;
}

std::string CFileStream::readString(size_t len){
	assert(mode == OpenMode::In);
    std::string str(len, '\0'); //creates string str at size of length and fills it with '\0'
    base.read(&str[0], len);
    return str;
}

std::string CFileStream::readWString(size_t len){
	assert(mode == OpenMode::In);
    std::string str(len, '\0'); //creates string str at size of length and fills it with '\0'
    base.read(&str[0], len);
    return str;
}

void CFileStream::writeInt8(int8_t v){
	assert(mode == OpenMode::Out);
	base.write((char*)&v, 1);
}

void CFileStream::writeUInt8(uint8_t v){
	assert(mode == OpenMode::Out);
	base.write((char*)&v, 1);
}

void CFileStream::writeInt16(int16_t v){
	assert(mode == OpenMode::Out);
	if(order != systemOrder){
		v = swap16(v);
	}
	base.write((char*)&v, sizeof(uint16_t));
}

void CFileStream::writeUInt16(uint16_t v){
	assert(mode == OpenMode::Out);
	if(order != systemOrder){
		v = swap16(v);
	}
	base.write((char*)&v, sizeof(uint16_t));
}

void CFileStream::writeInt32(int32_t v){
	assert(mode == OpenMode::Out);
	if(order != systemOrder){
	   v = swap32(v);
	}
	base.write((char*)&v, sizeof(int32_t));
}

void CFileStream::writeUInt32(uint32_t v){
	assert(mode == OpenMode::Out);
	if(order != systemOrder){
	   v = swap32(v);
	}
	base.write((char*)&v, sizeof(uint32_t));
}

void CFileStream::writeFloat(float v){
	assert(mode == OpenMode::Out);
	char* buff = (char*)&v;
	if(order != systemOrder){
		char temp[sizeof(float)];
		temp[0] = buff[3];
		temp[1] = buff[2];
		temp[2] = buff[1];
		temp[3] = buff[0];
		v = *((float*)temp);
	}
	base.write((char*)&v, sizeof(float));
}

void CFileStream::writeString(std::string v){
	assert(mode == OpenMode::Out);
	base.write(v.c_str(), v.size());
}

void CFileStream::writeBytes(char* v, size_t size){
	assert(mode == OpenMode::Out);
	base.write(v, size);
}

uint8_t CFileStream::peekU8(int offset){
	assert(mode == OpenMode::In);
	uint8_t ret;
	int pos = base.tellg();
	base.seekg(offset, base.beg);
	ret = readUInt8();
	base.seekg(pos, base.beg);
	return ret;
}

int8_t CFileStream::peekI8(int offset){
	assert(mode == OpenMode::In);
	int8_t ret;
	int pos = base.tellg();
	base.seekg(offset, base.beg);
	ret = readInt8();
	base.seekg(pos, base.beg);
	return ret;
}

uint16_t CFileStream::peekU16(int offset){
	assert(mode == OpenMode::In);
	uint16_t ret;
	int pos = base.tellg();
	base.seekg(offset, base.beg);
	ret = readUInt16();
	base.seekg(pos, base.beg);
	return ret;
}

int16_t CFileStream::peekI16(int offset){
	assert(mode == OpenMode::In);
	int16_t ret;
	int pos = base.tellg();
	base.seekg(offset, base.beg);
	ret = readInt16();
	base.seekg(pos, base.beg);
	return ret;
}

uint32_t CFileStream::peekU32(int offset){
	assert(mode == OpenMode::In);
	uint32_t ret;
	int pos = base.tellg();
	base.seekg(offset, base.beg);
	ret = readUInt32();
	base.seekg(pos, base.beg);
	return ret;
}

int32_t CFileStream::peekI32(int offset){
	assert(mode == OpenMode::In);
	int32_t ret;
	int pos = base.tellg();
	base.seekg(offset, base.beg);
	ret = readInt32();
	base.seekg(pos, base.beg);
	return ret;
}

size_t CFileStream::getSize(){
	int pos = base.tellg();
	base.seekg(0, std::ios::end);
	size_t ret = base.tellg();
	base.seekg(pos, std::ios::beg);
	return ret;
}

///
///
///  CMemoryStream
///
///


CMemoryStream::CMemoryStream(uint8_t* ptr, size_t size, Endianess ord, OpenMode mode){
	mBuffer = ptr;
	mPosition = 0;
	mSize = size;
	mCapacity = size;
	mHasInternalBuffer = false;
	mOpenMode = mode; 
	order = ord;
	systemOrder = getSystemEndianess();
}

CMemoryStream::CMemoryStream(size_t size, Endianess ord, OpenMode mode){
	mBuffer = new uint8_t[size];
	mPosition = 0;
	mSize = size;
	mCapacity = size;
	mHasInternalBuffer = true;
	mOpenMode = mode;
	order = ord;
	systemOrder = getSystemEndianess();
}

size_t CMemoryStream::getSize(){
	return mSize;
}

size_t CMemoryStream::getCapacity(){
	return mCapacity;
}

void CMemoryStream::seek(size_t pos){
	mPosition = (pos > mSize ? mPosition : pos);
}

uint8_t* CMemoryStream::getBuffer(){
	return mBuffer;
}

///
/// Memstream Reading Functions
///

int8_t CMemoryStream::readInt8(){
	assert(mOpenMode == OpenMode::In && mPosition < mSize);
	int8_t r;
	memcpy(&r, OffsetPointer<int8_t>(mBuffer, mPosition), sizeof(int8_t));
	mPosition++;
	return r;
}

uint8_t CMemoryStream::readUInt8(){
	assert(mOpenMode == OpenMode::In);
	uint8_t r;
	memcpy(&r, OffsetPointer<uint8_t>(mBuffer, mPosition), sizeof(uint8_t));
	mPosition++;
	return r;
}

int16_t CMemoryStream::readInt16(){
	assert(mOpenMode == OpenMode::In && mPosition < mSize);
	int16_t r;
	memcpy(&r, OffsetPointer<int16_t>(mBuffer, mPosition), sizeof(int16_t));
	mPosition += sizeof(int16_t);

	if(order != systemOrder){
		return swap16(r);
	}
	else{
		return r;
	}
}

uint16_t CMemoryStream::readUInt16(){
	assert(mOpenMode == OpenMode::In && mPosition < mSize);
	uint16_t r;
	memcpy(&r, OffsetPointer<uint16_t>(mBuffer, mPosition), sizeof(uint16_t));
	mPosition += sizeof(uint16_t);

	if(order != systemOrder){
		return swap16(r);
	}
	else{
		return r;
	}
}

uint32_t CMemoryStream::readUInt32(){
	assert(mOpenMode == OpenMode::In && mPosition < mSize);
	uint32_t r;
	memcpy(&r, OffsetPointer<uint32_t>(mBuffer, mPosition), sizeof(uint32_t));
	mPosition += sizeof(uint32_t);

	if(order != systemOrder){
		return swap32(r);
	}
	else{
		return r;
	}
}

int32_t CMemoryStream::readInt32(){
	assert(mOpenMode == OpenMode::In && mPosition < mSize);
	int32_t r;
	memcpy(&r, OffsetPointer<int32_t>(mBuffer, mPosition), sizeof(int32_t));
	mPosition += sizeof(int32_t);

	if(order != systemOrder){
		return swap32(r);
	}
	else{
		return r;
	}
}

std::string CMemoryStream::readString(size_t len){
	assert(mOpenMode == OpenMode::In && mPosition < mSize);
	std::string str(OffsetPointer<char>(mBuffer, mPosition), mPosition+len);
	mPosition += len;
	return str;
}

///
/// Memstream Writing Functions
///

//included in writing functions because this is needed when using an internal buffer
bool CMemoryStream::Reserve(size_t needed){
	if(mCapacity >= needed){
		return true;
	}
	if(!mHasInternalBuffer){
		return false;
	}

	mCapacity *= 2;
	uint8_t* temp = new uint8_t[mCapacity];
	memcpy(temp, mBuffer, mSize);
	delete[] mBuffer;
	mBuffer = temp;

	return true;
}

void CMemoryStream::writeInt8(int8_t v){
	Reserve(mPosition + sizeof(v));
	memcpy(OffsetWritePointer<int8_t>(mBuffer, mPosition), &v, sizeof(int8_t));
	mPosition += sizeof(int8_t);
}

void CMemoryStream::writeUInt8(uint8_t v){
	Reserve(mPosition + sizeof(v));
	memcpy(OffsetWritePointer<uint8_t>(mBuffer, mPosition), &v, sizeof(int8_t));
	mPosition += sizeof(int8_t);
}

void CMemoryStream::writeInt16(int16_t v){
	Reserve(mPosition + sizeof(v));
	memcpy(OffsetWritePointer<int16_t>(mBuffer, mPosition), &v, sizeof(int16_t));
	mPosition += sizeof(int16_t);
}

void CMemoryStream::writeUInt16(uint16_t v){
	Reserve(mPosition + sizeof(v));
	memcpy(OffsetWritePointer<uint16_t>(mBuffer, mPosition), &v, sizeof(int16_t));
	mPosition += sizeof(int16_t);
}

void CMemoryStream::writeInt32(int32_t v){
	Reserve(mPosition + sizeof(v));
	memcpy(OffsetWritePointer<int32_t>(mBuffer, mPosition), &v, sizeof(int32_t));
	mPosition += sizeof(int32_t);
}

void CMemoryStream::writeUInt32(uint32_t v){
	Reserve(mPosition + sizeof(v));
	memcpy(OffsetWritePointer<uint32_t>(mBuffer, mPosition), &v, sizeof(int32_t));
	mPosition += sizeof(int32_t);
}

void CMemoryStream::writeFloat(float v){
	Reserve(mPosition + sizeof(v));
	memcpy(OffsetWritePointer<float>(mBuffer, mPosition), &v, sizeof(float));
	mPosition += sizeof(float);
}


//TODO: Clean these up and test them more

void CMemoryStream::writeBytes(char* bytes, size_t size){
	Reserve(mPosition + size);
	memcpy(OffsetWritePointer<char>(mBuffer, mPosition), &bytes, size);
	mPosition += size;
}

void CMemoryStream::writeString(std::string str){
	Reserve(mPosition + str.size());
	memcpy(OffsetWritePointer<char>(mBuffer, mPosition), str.data(), str.size());
	mPosition += str.size();
}

}
#endif