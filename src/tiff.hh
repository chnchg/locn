/**\file
   \brief Class for processing TIFF file
   \author Chun-Chung Chen &lt;cjj@u.washington.edu&gt;
 */
#pragma once
#include <memory>
#include <fstream>
#include <vector>
#include <tuple>
#include <map>

/// TIFF file processor
class Tiff
{
	std::shared_ptr<std::ifstream> sp; ///<stream to read data from
	bool le; ///<little endian?
	bool efix; ///<need to fix endian?
	uint32_t ifd; ///<Image file directory offset
	// read from stream
	uint16_t read16();
	uint32_t read32();
public:
	/// Directory Entry
	struct DEntry
	{
		uint16_t tag; ///<entry name
		uint16_t type; ///<entry data type
		uint32_t count; ///<data count
		uint32_t data; ///<data or offset
	};
private:
	DEntry read_dentry(); // endian is corrected
	uint16_t get16(DEntry const & e) {return le?e.data&0xffff:e.data>>16;} // first uint16
	uint16_t next16(DEntry const & e) {return le?e.data>>16:e.data&0xffff;} // second uint16
	uint32_t to32(DEntry const & e);
	std::string get_str(DEntry const & de);
	std::vector<uint16_t> get16s(DEntry const & e);
	std::vector<uint32_t> get32s(DEntry const & e);	
	std::tuple<uint32_t,uint32_t> get_ratio(DEntry const & e);
public:
	/// Construct TIFF processor from an input stream
	Tiff(std::shared_ptr<std::ifstream> s) : sp(s) {}
	void start(); ///<start with TIFF file header
	/// Parse image file directory (IFD)
	uint32_t parse_ifd(
		uint32_t i = 0 ///<offset position for the IFD
	); ///<\return offset position for next IFD, 0 if there is no more
	std::vector<char> read_image(); ///<read image data
	/// Directory entry ID tags
	enum Tag {
		Tag_ImageWidth = 0x100,
		Tag_ImageLength = 0x101,
		Tag_BitsPerSample = 0x102,
		Tag_Compression = 0x103,
		Tag_PhotometricInterpretation = 0x106,
		Tag_FillOrder = 0x10a,
		Tag_ImageDescription = 0x10e,
		Tag_StripOffsets = 0x111,
		Tag_Orientation = 0x112,
		Tag_SamplesPerPixel = 0x115,
		Tag_RowsPerStrip = 0x116,
		Tag_StripByteCounts = 0x117,
		Tag_XResolution = 0x11a,
		Tag_YResolution = 0x11b,
		Tag_PlanarConfiguration = 0x11c,
		Tag_ResolutionUnit = 0x128,
		Tag_Software = 0x131,
		Tag_SampleFormat = 0x153,
		Tag_ImageID = 0x800d
	};
	/// Compression methods
	enum Compression {
		Cmp_None = 1,
		Cmp_CCITT = 2,
		Cmp_PackBits = 32773
	};
	/// String names for compression methods
	std::map<Compression,std::string> const compress_name {
		{Cmp_None,"None"},
		{Cmp_CCITT,"CCITT"},
		{Cmp_PackBits,"PackBits"}
	};
	/// Photometric formats
	enum Photometric {
		Ptm_WhiteIsZero = 0,
		Ptm_BlackIsZero = 1,
		Ptm_RGB = 2,
		Ptm_Palette = 3,
		Ptm_TransparencyMask = 4
	};
	/// String names for photometric formats
	std::map<Photometric,std::string> const photom_name {
		{Ptm_WhiteIsZero,"WhiteIsZero"},
		{Ptm_BlackIsZero,"BlackIsZero"},
		{Ptm_RGB,"RGB"},
		{Ptm_Palette,"Palette"},
		{Ptm_TransparencyMask,"TransparencyMask"}
	};
	/// Units for image
	enum Unit {
		Unt_None = 1,
		Unt_Inch = 2,
		Unt_Centimeter = 3
	};
	/// String names for units
	std::map<Unit,std::string> const unit_name {
		{Unt_None,"None"},
		{Unt_Inch,"Inch"},
		{Unt_Centimeter,"Centimeter"}
	};
	/// Sample formats
	enum SampleFormat {
		Fmt_Unsigned = 1,
		Fmt_TwoComplement = 2,
		Fmt_IEEEFloat = 3,
		Fmt_Undefined = 4
	};
	/// String names for sample formats
	std::map<SampleFormat,std::string> const format_name {
		{Fmt_Unsigned,"Unsigned"},
		{Fmt_TwoComplement,"TwoComplement"},
		{Fmt_IEEEFloat,"IEEEFloat"},
		{Fmt_Undefined,"Undefined"}
	};
	uint32_t image_width; ///<width of image
	uint32_t image_length; ///<length of image
	std::vector<uint16_t> bits_per_sample; ///<bits per sample
	Compression compression; ///<compression method
	Photometric photometric; ///<photometric format
	uint16_t fill_order; ///<fill order
	std::string image_description; ///<image description
	std::vector<uint32_t> strip_offsets; ///<strip offsets
	uint16_t orientation; ///<orientation of image
	uint16_t samples_per_pixel; ///<samples per pixel
	uint32_t rows_per_strip; ///<rows per strip
	std::vector<uint32_t> strip_byte_counts; ///<strip byte counots
	std::tuple<uint32_t,uint32_t> xresolution; ///<x resolution
	std::tuple<uint32_t,uint32_t> yresolution; ///<y resolution
	uint16_t planar_configuration; ///<planar configuration
	Unit resolution_unit; ///<unit for image resolution
	std::string software; ///<software used
	std::vector<SampleFormat> sample_formats; ///<sample formats
	std::string image_id; ///<image ID
};
