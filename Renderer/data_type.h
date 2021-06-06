#pragma once

#include "glad.h"
#include <cstdint>

enum class DataType {
	Int = 0,
	Short,
	Float,
	Byte,
	UInt,
	UShort,
	UByte
};

static size_t DataSizes[] = {
	4,
	2,
	4,
	1,
	4,
	2,
	1
};

static size_t DataOpenGLMap[] = {
	GL_INT,
	GL_SHORT,
	GL_FLOAT,
	GL_BYTE,
	GL_UNSIGNED_INT,
	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_BYTE
};
