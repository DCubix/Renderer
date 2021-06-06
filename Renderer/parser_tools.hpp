#pragma once

#include <cctype>
#include <list>
#include <string>
#include <functional>
#include <regex>
#include <algorithm>

class Scanner {
public:
	Scanner() = default;
	~Scanner() = default;

	Scanner(const std::string& data) {
		m_data = std::list<char>(data.begin(), data.end());
	}

	bool hasNext() const { return !m_data.empty(); }
	
	char peek() const { return hasNext() ? m_data.front() : '\0'; }
	char scan() {
		if (!hasNext()) return '\0';
		char c = peek();
		m_data.pop_front();
		return c;
	}

	std::string scanWhileMatch(const std::function<bool(char)>& matcher, bool includeLast = false) {
		std::string ret;
		while (matcher(peek()) && hasNext()) {
			ret += scan();
		}
		if (includeLast) ret += scan();
		return ret;
	}

	std::string scanRegEx(const std::string& regex) {
		std::regex re(regex);
		return scanWhileMatch([&](char c) {
			return std::regex_match(std::string(1, c), re);
		});
	}

	int scanInt() {
		return std::stoi(scanWhileMatch([](char c) { return isdigit(c); }));
	}

	float scanFloat() {
		return std::stoi(scanWhileMatch([](char c) { return isdigit(c) || c == '.'; }));
	}

private:
	std::list<char> m_data;
};