#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <curl/curl.h>
#include <json/json.h>

std::string API_KEY = "your-api-key";
const std::string DEEPL_API_URL = "https://api-free.deepl.com/v2/translate";

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

bool contains_japanese(const std::string& input) {
	for (const unsigned char& c : input) {
		if ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xEF)) {
			return true;
		}
	}
	return false;
}

std::string translate(const std::string& input) {
	CURL* curl = curl_easy_init();
	std::string response;

	if (curl) {
		char* escaped_input = curl_easy_escape(curl, input.c_str(), 0);
		std::string query = "text=" + std::string(escaped_input) + "&target_lang=EN";
		curl_free(escaped_input);

		struct curl_slist* headers = NULL;

		headers = curl_slist_append(headers, ("Authorization: DeepL-Auth-Key " + API_KEY).c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, DEEPL_API_URL.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	Json::Value jsonData;
	Json::CharReaderBuilder jsonReader;
	std::string errs;

	std::istringstream iss(response);
	if (!Json::parseFromStream(jsonReader, iss, &jsonData, &errs)) {
		return "Error: Failed to parse API response.";
	}

	return jsonData["translations"][0]["text"].asString();
}

std::string translate_japanese_substrings(const std::string& line) {
	std::regex japanese_regex("([\\x81-\\x9F\\xE0-\\xEF][\\x40-\\x7E\\x80-\\xFC][^\\x00-\\x7F]*)([^\\x81-\\x9F\\xE0-\\xEF\\x40-\\x7E\\x80-\\xFC]*)([\\x81-\\x9F\\xE0-\\xEF][\\x40-\\x7E\\x80-\\xFC][^\\x00-\\x7F]*)");
	std::smatch match;
	std::string processed_line = line;

	while (std::regex_search(processed_line, match, japanese_regex)) {
		std::string japanese_text = match.str();
		std::string translated_text = translate(japanese_text);
		processed_line = match.prefix().str() + translated_text + match.suffix().str();
	}

	return processed_line;
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << " <api_key> <input_file> <output_file>" << std::endl;
		return 1;
	}

	API_KEY = argv[1];
	
	std::ifstream file(argv[2]);
	if (!file) {
		std::cerr << "Error: Could not open input file." << std::endl;
		return 1;
	}

	std::ofstream output_file(argv[3]);
	if (!output_file) {
		std::cerr << "Error: Could not open output file." << std::endl;
		return 1;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::string processed_line = translate_japanese_substrings(line);
		std::cout << "Original: " << line << std::endl;
		std::cout << "Processed: " << processed_line << std::endl;
		output_file << processed_line << std::endl;
		std::cout << std::endl;
	}

	file.close();
	output_file.close();
	return 0;
}