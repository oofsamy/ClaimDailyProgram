#include "cpr/cpr.h"
#include "json.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <windows.h>
#include <shellapi.h>

const std::string TOKEN_ENDPOINT = "https://account-public-service-prod.ol.epicgames.com/account/api/oauth/token";
const std::string EXCHANGE_CODE_ENDPOINT = "https://account-public-service-prod.ol.epicgames.com/account/api/oauth/exchange";

template<typename T>
void print(T s) { std::cout << s << std::endl; }

struct FRefreshTokens {
	std::string ID;
	std::string AccessToken;
	std::string RefreshToken;
};

struct FToken {
	std::string ID;
	std::string AccessToken;
};

void JsonFileExists() {
	if (!std::filesystem::exists("Info.json")) {
		std::ofstream NewFile("Info.json");
		NewFile << R"({"access_token":"undefined", "account_id" : "undefined", "refresh_token" : "undefined"})";
		NewFile.close();
	}
}

std::string ReadContent(std::string FileName) {
	std::ifstream Temp(FileName);
	std::stringstream Buffer;
	Buffer << Temp.rdbuf();

	return (Buffer.str());
}

bool CheckForValues() {

	std::string Result = ReadContent("Info.json");
	auto ResultJSON = nlohmann::json::parse(Result);
	
	if (ResultJSON["account_id"] == "undefined" || ResultJSON["access_token"] == "undefined" || ResultJSON["refresh_token"] == "undefined") {
		return true;
	}
	
	return false;
}

void WriteContent(std::string FileName, std::string Buffer) {
	std::ofstream File(FileName, std::ofstream::trunc);
	File << Buffer;
	File.close();
}

FRefreshTokens GetTokensFromFile() {
	auto Result = nlohmann::json::parse(ReadContent("Info.json"));

	return { Result["account_id"], Result["access_token"], Result["refresh_token"] };
}

void SetTokens(FRefreshTokens Token) {
	auto Result = nlohmann::json::parse(ReadContent("Info.json"));

	Result = {
		{"account_id", Token.ID},
		{"refresh_token", Token.RefreshToken},
		{"access_token", Token.AccessToken}
	};

	WriteContent("Info.json", Result.dump());
}

FRefreshTokens GetRefreshTokenFromAuthCode(std::string AuthorizationCode) {
	cpr::Response r = cpr::Post(cpr::Url{ TOKEN_ENDPOINT },
								cpr::Payload{ { "grant_type", "authorization_code"}, {"code", AuthorizationCode}},
								cpr::Header{ {"Content-Type", "application/x-www-form-urlencoded"}, {"Authorization", "basic MzRhMDJjZjhmNDQxNGUyOWIxNTkyMTg3NmRhMzZmOWE6ZGFhZmJjY2M3Mzc3NDUwMzlkZmZlNTNkOTRmYzc2Y2Y="} });

	auto Result = nlohmann::json::parse(r.text);

	return FRefreshTokens{ Result["account_id"], Result["access_token"], Result["refresh_token"] };
}

FRefreshTokens RefreshTokenFunc(FRefreshTokens RToken) {
	cpr::Response r = cpr::Post(cpr::Url{ TOKEN_ENDPOINT },
								cpr::Payload{ { "grant_type", "refresh_token"}, {"refresh_token", RToken.RefreshToken} },
								cpr::Header{ {"Content-Type", "application/x-www-form-urlencoded"}, {"Authorization", "basic MzRhMDJjZjhmNDQxNGUyOWIxNTkyMTg3NmRhMzZmOWE6ZGFhZmJjY2M3Mzc3NDUwMzlkZmZlNTNkOTRmYzc2Y2Y="} });

	auto Result = nlohmann::json::parse(r.text);

	FRefreshTokens Token = { Result["account_id"], Result["access_token"], Result["refresh_token"] };

	SetTokens(Token);
	
	return {Token};
}

std::string GetExchangeCode(FRefreshTokens RToken) {
	cpr::Response r = cpr::Get(cpr::Url{ EXCHANGE_CODE_ENDPOINT },
								cpr::Header{ {"Authorization", "bearer " + RToken.AccessToken}});

	return nlohmann::json::parse(r.text)["code"];
}

std::string GetExchangeCode(FToken Token) {
	cpr::Response r = cpr::Get(cpr::Url{ EXCHANGE_CODE_ENDPOINT },
								cpr::Header{ {"Authorization", "bearer " + Token.AccessToken} });

	return nlohmann::json::parse(r.text)["code"];
}

FToken GetFortnitePCGameClientToken(std::string ExchangeCode) {
	cpr::Response r = cpr::Post(cpr::Url{ TOKEN_ENDPOINT },
								cpr::Payload{ { "grant_type", "exchange_code"}, {"exchange_code", ExchangeCode} },
								cpr::Header{ {"Content-Type", "application/x-www-form-urlencoded"}, {"Authorization", "basic MzQ0NmNkNzI2OTRjNGE0NDg1ZDgxYjc3YWRiYjIxNDE6OTIwOWQ0YTVlMjVhNDU3ZmI5YjA3NDg5ZDMxM2I0MWE="} });

	auto Result = nlohmann::json::parse(r.text);

	return {Result["account_id"], Result["access_token"]};
}

void RedeemReward(FToken Token) {
	cpr::Response r = cpr::Post(cpr::Url{ "https://fortnite-public-service-prod11.ol.epicgames.com/fortnite/api/game/v2/profile/" + Token.ID + "/client/ClaimLoginReward?profileId=campaign" },
								cpr::Body{ "{}" },
								cpr::Header{ {"Content-Type", "application/json"}, {"Authorization", "Bearer "+Token.AccessToken} });

	WriteContent("Debug.txt", r.text);
}

int main() {
	JsonFileExists();
	if (CheckForValues()) {
		std::cout << "Please enter an authorization token by: ";
		ShellExecute(0, 0, L"https://www.epicgames.com/id/logout?redirectUrl=https%3A//www.epicgames.com/id/login%3FredirectUrl%3Dhttps%253A%252F%252Fwww.epicgames.com%252Fid%252Fapi%252Fredirect%253FclientId%253D34a02cf8f4414e29b15921876da36f9a%2526responseType%253Dcode", 0, 0, SW_SHOW);
		std::string AuthorizationCode = "";
		std::cin >> AuthorizationCode;

		FRefreshTokens RToken = GetRefreshTokenFromAuthCode(AuthorizationCode);
		SetTokens(RToken);
		FToken Token = GetFortnitePCGameClientToken(GetExchangeCode(RToken));
		RedeemReward(Token);
	} else {
		FRefreshTokens RToken = RefreshTokenFunc(GetTokensFromFile());
		SetTokens(RToken);
		GetExchangeCode(RToken);
		FToken Token = GetFortnitePCGameClientToken(GetExchangeCode(RToken));
		RedeemReward(Token);
	}

	return 0;
}