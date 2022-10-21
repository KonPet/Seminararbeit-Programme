#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "fpng.h"
#include "qoixx.hpp"

double resDP = 0;
double resEP = 0;
double resDQ = 0;
double resEQ = 0;
double resSP = 0;
double resSQ = 0;

double tresDP = 0;
double tresEP = 0;
double tresDQ = 0;
double tresEQ = 0;
double tresSP = 0;
double tresSQ = 0;

double log(uint32_t a, uint32_t b = 1024) {
	return log2(a) / log2(b);
}

int reps = 1;

int pFaster = 0;
int pDeFaster = 0;
int qFaster = 0;
int qDeFaster = 0;
int pBComp = 0;
int qBComp = 0;

int fSame = 0;
int eSame = 0;
int dSame = 0;

int gcount = 0;

void test(std::ofstream& statFile, std::string in, std::string ext = "png") {
//	std::cout << in + "." + ext << std::endl;

	// Load the image
	int w, h, c;
	unsigned char* img = stbi_load((in + ext).c_str(), &w, &h, &c, 0);

	if (c != 3 && c != 4) {
		stbi_image_free(img);
		img = stbi_load((in + ext).c_str(), &w, &h, &c, 3);
		c = 3;
	}

	// PNG
	auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < reps; i++) {
		fpng::fpng_encode_image_to_file("out.png", img, w, h, c);
	}
	auto end = std::chrono::steady_clock::now();

	std::chrono::duration<double, std::milli> elapsed_seconds = end-start;
	auto pngEncTime = elapsed_seconds / reps;
	resEP += pngEncTime.count();

	std::vector<uint8_t> imgPLoad;
	start = std::chrono::steady_clock::now();
	for (int i = 0; i < reps; i++) {
		uint32_t pw, ph, pc;
		fpng::fpng_decode_file("out.png", imgPLoad, pw, ph, pc, c);
	}
	end = std::chrono::steady_clock::now();

	elapsed_seconds = end-start;
	auto pngDecTime = elapsed_seconds / reps;
	resDP += pngDecTime.count();

	std::vector<unsigned char> imgV;
	imgV.insert(imgV.end(), &img[0], &img[w*h*c]);

	// QOI
	start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < reps; i++) {

		auto desc = qoixx::qoi::desc{
				.width = static_cast<std::uint32_t>(w),
				.height = static_cast<std::uint32_t>(h),
				.channels = static_cast<std::uint8_t>(c),
				.colorspace = qoixx::qoi::colorspace::srgb
		};

		auto testasd = qoixx::qoi::encode<std::vector<uint8_t>>((const uint8_t*) img, w * h * c, desc);

		std::ofstream out("out.qoi", std::ios::binary);
		out.write(reinterpret_cast<const char*>(testasd.data()), testasd.size());
		out.close();
	}

	end = std::chrono::high_resolution_clock::now();

	elapsed_seconds = end-start;
	auto qoiTime = elapsed_seconds / reps;
	resEQ += qoiTime.count();

	start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < reps; i++) {
		std::vector<unsigned char> qoiDecode;
		size_t fSize = std::filesystem::file_size("out.qoi");
		std::ifstream inFile("out.qoi", std::ios::binary);
		qoiDecode.resize(fSize);
		inFile.read((char*) qoiDecode.data(), fSize);
		inFile.close();
		qoixx::qoi::decode<std::vector<uint8_t>>(qoiDecode.data(), fSize, c);
//		qoi::decode(qoiDecode);
	}

	end = std::chrono::high_resolution_clock::now();

	elapsed_seconds = end-start;
	auto qoiDecTime = elapsed_seconds / reps;
	resDQ += qoiDecTime.count();

//	std::cout << "**" << in << "**" << std::endl;
//	std::cout << "\n| Format | Time taken (Dec) | Time taken (Enc) | Space used |\n| --- | --- | --- | --- |" << std::endl;

	uint32_t pngSpace = std::filesystem::file_size("out.png");
	double exp = (int) log(pngSpace, 1024);
	const char* units[] = {"B", "KB", "MB", "GB"};
//	std::cout << "| PNG | " << pngDecTime << " | " << pngEncTime << " | " << pngSpace / pow(1024, exp) << " " << units[(int) exp] << " |" << std::endl;
	statFile << "| PNG | " << pngDecTime << " | " << pngEncTime << " | " << pngSpace / pow(1024, exp) << " " << units[(int) exp] << " |\n";

	uint32_t qoiSpace = std::filesystem::file_size("out.qoi");
//	std::cout << "| QOI | " << qoiDecTime << " | " << qoiTime << " | " << qoiSpace / pow(1024, exp) << " " << units[(int) exp] << " |\n" << std::endl;
	statFile << "| QOI | " << qoiDecTime << " | " << qoiTime << " | " << qoiSpace / pow(1024, exp) << " " << units[(int) exp] << " |\n\n";

	resSQ += std::filesystem::file_size("out.qoi");
	resSP += std::filesystem::file_size("out.png");

	std::filesystem::remove("out.png");
	std::filesystem::remove("out.qoi");

	(pngSpace < qoiSpace ? pBComp : (qoiSpace < pngSpace ? qBComp : fSame))++;
	(pngDecTime < qoiDecTime ? pDeFaster : (qoiDecTime < pngDecTime ? qDeFaster : dSame))++;
	(pngEncTime < qoiTime ? pFaster : (qoiTime < pngEncTime ? qFaster : eSame))++;

//	std::cout << "Space ratio: " << (double) std::filesystem::file_size((in + "_out.qoi").c_str()) / std::filesystem::file_size((in + "_out.png").c_str()) << std::endl;

	// End
	stbi_image_free(img);
}

int main() {
	fpng::fpng_init();

//	for (stbi_write_png_compression_level = 0; stbi_write_png_compression_level < 13; stbi_write_png_compression_level++) {
//		test("test");
//		test("hatsune");
//		test("street");
//	}

	int it = 0;

	for (const auto& entry : std::filesystem::directory_iterator("images")) {
		auto folderName = entry.path().filename().string();
		std::ofstream groupStats(("stats/" + folderName + ".md").c_str(), std::ios::trunc);

		for (const auto& imgPath : std::filesystem::directory_iterator(("images/" + folderName))) {
			if (imgPath.path().extension().string() == ".txt" || imgPath.path().extension().string() == ".TXT" || imgPath.path().extension().string() == ".php") continue;
//			std::cout << it++ << " (" << folderName << "/" << imgPath.path().stem() << ")" << std::endl;

			groupStats << "**" << folderName << "/" << imgPath.path().filename().string() <<
				"**\n\n| Format | Time taken (Decoder) | Time taken (Encoder) | Space used |\n| --- | --- | --- | --- |\n";
//			std::cout << "**" << folderName << "/" << imgPath.path().filename().string() <<
//			          "**\n\n| Format | Time taken (Decoder) | Time taken (Encoder) | Space used |\n| --- | --- | --- | --- |\n";

			test(groupStats, "images/" + folderName + "/" + imgPath.path().stem().string(), imgPath.path().extension().string());
			gcount++;
		}

		std::cout << folderName << std::endl;
		std::cout << std::endl;
		std::cout << resEP / resEQ << std::endl;
		std::cout << resDP / resDQ << std::endl;
		std::cout << resSP / resSQ << std::endl;

		std::cout << std::endl;

		std::cout << resEP / gcount << " " << resEQ / gcount << std::endl;
		std::cout << resDP / gcount << " " << resDQ / gcount << std::endl;
		std::cout << resSP / gcount << " " << resSQ / gcount << std::endl;

//		std::cout << std::endl;
		std::cout << "====================" << std::endl;

		tresEP += resEP;
		tresEQ += resEQ;
		tresDP += resDP;
		tresDQ += resDQ;
		tresSP += resSP;
		tresSQ += resSQ;

		resEP = 0;
		resEQ = 0;
		resDP = 0;
		resDQ = 0;
		resSP = 0;
		resSQ = 0;

		gcount = 0;

		groupStats.close();
	}

	std::cout << tresEP / tresEQ << std::endl;
	std::cout << tresDP / tresDQ << std::endl;
	std::cout << tresSP / tresSQ << std::endl;

	std::cout << std::endl;

	std::cout << tresEP << " " << tresEQ << std::endl;
	std::cout << tresDP << " " << tresDQ << std::endl;
	std::cout << tresSP << " " << tresSQ << std::endl;

	std::cout << std::endl;

	std::cout << pBComp << " " << qBComp << " " << fSame << std::endl;
	std::cout << pDeFaster << " " << qDeFaster << " " << dSame << std::endl;
	std::cout << pFaster << " " << qFaster << " " << eSame << std::endl;

	return 0;
}
