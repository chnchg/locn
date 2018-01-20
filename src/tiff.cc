///\file
#include "tiff.hh"
#include "utils.hh"
#include <functional>
#include <iostream>

using namespace std;

namespace {union T {unsigned i;char c;};}
constexpr bool is_little() {return T({1}).c == '\1';} ///<is little endian

///@{
inline void tcheck(Tiff::DEntry const & e, uint16_t t)
{
	if (e.type!=t) throw Error("Entry type error");
}
///@}

uint16_t Tiff::read16()
{
	union {
		uint16_t v;
		char c[2];
	} b;
	sp->read(b.c,2);
	if (efix) {
		char t = b.c[1];
		b.c[1] = b.c[0];
		b.c[0] = t;
	}
	return b.v;
}

uint32_t Tiff::read32()
{
	union {
		uint32_t v;
		char c[4];
	} b;
	sp->read(b.c,4);
	if (efix) {
		char t = b.c[3];
		b.c[3] = b.c[0];
		b.c[0] = t;
		t = b.c[2];
		b.c[2] = b.c[1];
		b.c[1] = t;
	}
	return b.v;
}

Tiff::DEntry Tiff::read_dentry()
{
	DEntry e;
	e.tag = read16();
	e.type = read16();
	e.count = read32();
	e.data = read32();
	return e;
}

uint32_t Tiff::to32(DEntry const & e)
{
	if (e.type==4) return e.data;
	tcheck(e,3);
	return get16(e);
}

std::string Tiff::get_str(DEntry const & e)
{
	tcheck(e,2);
	std::vector<char> b(e.count);
	sp->seekg(e.data);
	sp->read(b.data(),e.count);
	if (b[e.count-1]!='\0') warn << "String does not end with '\\0'";
	return std::string(b.data(),e.count);
}

std::vector<uint16_t> Tiff::get16s(DEntry const & e)
{
	std::vector<uint16_t> r;
	tcheck(e,3);
	if (e.count<=2) {
		r.push_back(get16(e));
		if (e.count==2) r.push_back(next16(e));
	}
	else {
		sp->seekg(e.data);
		for (unsigned i = 0; i<e.count; i++) r.push_back(read16());
	}
	return r;
}

std::vector<uint32_t> Tiff::get32s(DEntry const & e)
{
	std::vector<uint32_t> r;
	tcheck(e,4);
	if (e.count<=1) r.push_back(e.data);
	else {
		sp->seekg(e.data);
		for (unsigned i = 0; i<e.count; i++) r.push_back(read32());
	}
	return r;
}

std::tuple<uint32_t,uint32_t> Tiff::get_ratio(DEntry const & e)
{
	tcheck(e,5);
	sp->seekg(e.data);
	return {read32(),read32()};
}

void Tiff::start()
{
	sp->seekg(0);
	char b[3];
	sp->read(b,2);
	b[2] = 0;
	le = string(b) == "II"; // little endian?
	efix = le^is_little();
	auto check = read16();
	debug << '[' << b << "]:" << check << '\n';
	ifd = read32();
	debug << "IFD at " << ifd << '\n';
}

uint32_t Tiff::parse_ifd(uint32_t i)
{
	sp->seekg(i?i:ifd);
	auto nde = read16();
	debug << "# dentry = " << nde << '\n';
	vector<DEntry> delist;
	for (unsigned i = 0; i < nde; i ++) {
		auto e = read_dentry();
		delist.push_back(e);
	}
	uint32_t ni = read32();
	static map<Tag,function<void(DEntry&)> > ptag = {
		{Tag_ImageWidth,[this](DEntry& e){image_width = to32(e);}},
		{Tag_ImageLength,[this](DEntry& e){image_length = to32(e);}},
		{Tag_BitsPerSample,[this](DEntry& e){bits_per_sample = get16s(e);}},
		{Tag_Compression,[this](DEntry& e){compression = (Compression)get16(e);}},
		{Tag_PhotometricInterpretation,[this](DEntry& e){photometric = (Photometric)get16(e);}},
		{Tag_FillOrder,[this](DEntry& e){fill_order = get16(e);}},
		{Tag_ImageDescription,[this](DEntry& e){image_description = get_str(e);}},
		{Tag_StripOffsets,[this](DEntry& e){strip_offsets = get32s(e);}},
		{Tag_Orientation,[this](DEntry& e){orientation = get16(e);}},
		{Tag_SamplesPerPixel,[this](DEntry& e){samples_per_pixel = to32(e);}},
		{Tag_RowsPerStrip,[this](DEntry& e){rows_per_strip = to32(e);}},
		{Tag_StripByteCounts,[this](DEntry& e){strip_byte_counts = get32s(e);}},
		{Tag_XResolution,[this](DEntry& e){xresolution = get_ratio(e);}},
		{Tag_YResolution,[this](DEntry& e){yresolution = get_ratio(e);}},
		{Tag_PlanarConfiguration,[this](DEntry& e){planar_configuration = get16(e);}},
		{Tag_ResolutionUnit,[this](DEntry& e){resolution_unit = (Unit)get16(e);}},
		{Tag_Software,[this](DEntry& e){software = get_str(e);}},
		{Tag_SampleFormat,[this](DEntry& e){for (auto i:get16s(e)) sample_formats.push_back((SampleFormat)i);}},
		{Tag_ImageID,[this](DEntry& e){image_id = get_str(e);}}
	};
	for (auto e: delist) {
		auto i = ptag.find((Tag)e.tag);
		if (i!=ptag.end()) i->second(e);
		else info << "unprocessed tag:" << e.tag << '\n';
	}
	debug << " next IFD: " << ni << '\n';
	return ni;
}

std::vector<char> Tiff::read_image()
{
	if (samples_per_pixel!=1 || bits_per_sample[0]!=16) {
		error("Unprocessed samples_per_pixel or bits_per_sample");
	}
	size_t isz = image_width*image_length*2;
	// std::cout << "isz = " << isz << '\n';
	std::vector<char> b(isz);
	size_t ns = (image_length+rows_per_strip-1)/rows_per_strip; // number of strips
	if (ns!=strip_offsets.size()) error("mismatch number of strip_offsets");
	size_t tsz = 0;
	for (unsigned i = 0; i<ns; i++) {
		size_t z = strip_byte_counts[i];
		sp->seekg(strip_offsets[i]);
		sp->read(b.data()+tsz, z);
		tsz += z;
	}
	if (tsz!=isz) error("Image byte size mismatch");
	if (efix) for (size_t i = 0; i<isz; i += 2) {
		char t = b[i];
		b[i] = b[i+1];
		b[i+1] = t;
	}
	return b;
}
