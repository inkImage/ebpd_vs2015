#ifndef _FILE_IO_H
#define _FILE_IO_H
#include "basic_types.h"
#include <iostream>
class Sample;

namespace FileIO
{
	enum FILE_TYPE{ XYZ,PLY,OBJ,OFF,NONE,PCMSCENE };

	Sample* load_point_cloud_file( std::string filename, FILE_TYPE type);
	bool load_point_cloud_file(Sample*);
	bool load_point_cloud_file(Sample*, std::string filename ,FILE_TYPE type);
	bool lazy_load_point_cloud_file(Sample*);
	Sample* lazy_load_point_cloud_file(std::string filename, FILE_TYPE type);
	bool saveFile(std::string,FILE_TYPE type ,IndexType smp_idx);
}

#endif