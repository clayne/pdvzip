// ZIP file has been moved to another location. We need to find and adjust the ZIP file record offsets to their new location.
void adjustZipOffsets(std::vector<uint8_t>& Vec, const uint_fast32_t VEC_SIZE, const uint_fast32_t LAST_IDAT_INDEX) {
	constexpr uint_fast8_t 
		ZIP_SIG[] 			{ 0x50, 0x4B, 0x03, 0x04 },
		START_CENTRAL_DIR_SIG[] 	{ 0x50, 0x4B, 0x01, 0x02 },
		START_CENTRAL_SIG_LENGTH 	= 4,
		CENTRAL_LOCAL_INDEX_DIFF 	= 45,
		END_CENTRAL_DIR_INDEX_DIFF 	= 38,
		END_CENTRAL_START_INDEX_DIFF 	= 19,
		ZIP_COMMENT_LENGTH_INDEX_DIFF 	= 21,
		ZIP_RECORDS_INDEX_DIFF 		= 11,
		ZIP_OFFSET_INDEX_DIFF 		= 4,
		INCREMENT_SEARCH_INDEX 		= 1,
		PNG_IEND_LENGTH 		= 16,
		BYTE_LENGTH 			= 2;

	const uint_fast32_t END_CENTRAL_DIR_INDEX = VEC_SIZE - END_CENTRAL_DIR_INDEX_DIFF;
	
	uint_fast32_t 
		total_zip_records_index = END_CENTRAL_DIR_INDEX + ZIP_RECORDS_INDEX_DIFF,
		end_central_start_index = END_CENTRAL_DIR_INDEX + END_CENTRAL_START_INDEX_DIFF,
		zip_comment_length_index = END_CENTRAL_DIR_INDEX + ZIP_COMMENT_LENGTH_INDEX_DIFF,
		zip_record_offset_index = LAST_IDAT_INDEX + ZIP_OFFSET_INDEX_DIFF,
		start_central_dir_index = 0;
	
	uint_fast16_t 
		total_zip_records = getByteValue(Vec, total_zip_records_index, BYTE_LENGTH, false),
		zip_comment_length = getByteValue(Vec, zip_comment_length_index, BYTE_LENGTH, false) + PNG_IEND_LENGTH, // Get and extend comment length to include end bytes of PNG. Important for JAR files.
		record_count = 0;
	
	uint_fast8_t value_bit_length = 16;

	valueUpdater(Vec, zip_comment_length_index, zip_comment_length, value_bit_length, false); // Update extended comment length value. 
		
	// Find the first start central directory index location by reverse iterating over the vector, starting from the end of the ZIP/IMAGE contents.
	// Seems safer than searching from the start of the ZIP, as there's a chance of a false SIG match for start_central_dir, especially with large files.
	for (uint_fast32_t index = VEC_SIZE - START_CENTRAL_SIG_LENGTH; record_count != total_zip_records; --index) {
		if (std::equal(std::begin(START_CENTRAL_DIR_SIG), std::end(START_CENTRAL_DIR_SIG), Vec.begin() + index)) {
			start_central_dir_index = index;
			record_count++;
		}
	}	
	
	uint_fast32_t central_dir_local_offset_index = start_central_dir_index + CENTRAL_LOCAL_INDEX_DIFF;

	value_bit_length = 32;

	valueUpdater(Vec, end_central_start_index, start_central_dir_index, value_bit_length, false);
	
	while (total_zip_records--) {
		valueUpdater(Vec, central_dir_local_offset_index, zip_record_offset_index, value_bit_length, false);
		zip_record_offset_index = searchFunc(Vec, zip_record_offset_index, INCREMENT_SEARCH_INDEX, ZIP_SIG);
		central_dir_local_offset_index = searchFunc(Vec, central_dir_local_offset_index, INCREMENT_SEARCH_INDEX, START_CENTRAL_DIR_SIG) + CENTRAL_LOCAL_INDEX_DIFF;
	}
}
