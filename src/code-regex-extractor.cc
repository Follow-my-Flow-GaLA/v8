#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <algorithm> // For std::find

// namespace v8 {
// namespace internal {

class CodeRegexExtractor {
private:
    std::vector<std::string> keywords = {
        "abstract", "arguments",  "boolean",  "break",
        "byte",     "case",       "catch",    "char",
        "const",    "continue",   "debugger", "default",
        "delete",   "do",         "double",   "else",
        "eval",	    "false",      "final",    "finally",
        "float",    "for",        "function", "goto",
        "if",       "implements", "in",       "instanceof",
        "int",      "interface",  "let",      "long",
        "native",   "new",        "null",     "package",
        "private",  "protected",  "public",   "return",
        "short",    "static",     "switch",   "synchronized",
        "this",     "throw",      "throws",   "transient",
        "true",     "try",        "typeof",   "var",
        "void",     "volatile",   "while",    "with",
        "yield",    "class",      "enum",     "export",
        "extends",  "import",     "super"
    };

    std::vector<std::string> func_names = {
        "eval",              "isFinite",      "isNaN",
        "parseFloat",        "parseInt",      "encodeURI",
        "encodeURIComponent","decodeURI",     "decodeURIComponent",
        "escape",            "unescape",      "uneval",
        "length",            "constructor",   "anchor",
        "big",               "blink",         "bold",
        "charAt",            "charCodeAt",    "codePointAt",
        "concat",            "endsWith",      "fontcolor",
        "fontsize",          "fixed",         "includes",
        "indexOf",           "italics",       "lastIndexOf",
        "link",              "localeCompare", "match",
        "matchAll",          "normalize",     "padEnd",
        "padStart",          "repeat",        "replace",
        "replaceAll",        "search",        "slice",
        "small",             "split",         "strike",
        "sub",               "substr",        "substring",
        "sup",               "startsWith",    "toString",
        "trim",              "trimStart",     "trimLeft",
        "trimEnd",           "trimRight",     "toLocaleLowerCase",
        "toLocaleUpperCase", "toLowerCase",   "toUpperCase",
        "valueOf",           "at"
    };

    std::vector<std::string> endmark_list = {
        "\\n",
        "...\\n\" ) ),",
        "...\\n\\n"
    };

    std::regex string_regex = std::regex("('.*?'|\".*?\"|\\\".*?\\\"|`.*?`|/.*?/[dgimsuy]?|(?:'|\\\"|`|/).*\\Z)");
    // std::regex variable_regex = std::regex("([^a-zA-Z0-9_$]([a-zA-Z_$][\\w$]*)\\b)");
    std::regex variable_regex = std::regex("([a-zA-Z_$][\\w$]*\\b)");
    std::regex split_regex = std::regex("([a-zA-Z_$][\\w$]*|\\\\s|\\s)");
    // std::regex endmark_regex = std::regex("(| ...\\\\n\\\" \\) \\),| ...\\\\n\\\\n)");
    std::regex endmark_regex = std::regex("...\\\\n\\\\n");
    // TODO: put comment in front of hex char
    std::regex comment_regex = std::regex("(//.*\\\\\\\\x0a|\\\\x0a|\\x0a|//.*\\.\\.\\.\\\\n\\\\n|//.*\\.\\.\\.\\\\n\\\" \\) \\),)");
    std::regex hexchar_regex = std::regex("(?:[^_]|^)((0|\\\\+)[xX][0-9a-fA-F]{1,8})"); // four backslashs in regex is one backslash in actual input
    std::regex space_regex = std::regex("([\\t\\n\\r\\f\\v]+| [ ]+|\\\\x09)");
    // std::regex clean_space_regex = std::regex("(. [^$\\w]+ .?|[^$\\w]+ .?|.? [^$\\w]+)");
    std::regex clean_space_regex = std::regex("(. [^$\\w]+ .?|[^$\\w]+ .?|.? [^$\\w]+)");
    std::vector<std::string> strings;
    std::vector<std::string> var_list;
    std::vector<std::string> words;
    std::string snippet;
    std::string code_regex;

public:
    CodeRegexExtractor() {
        // std::string string_pattern = "('.*?'|\".*?\"|\\\".*?\\\"|`.*?`|/.*?/[dgimsuy]?|(?:'|\\\"|`|/).*\\Z)";
        // std::regex string_regex(string_pattern);

        // std::string variable_pattern = "([^a-zA-Z0-9_$]([a-zA-Z_$][\\w$]*)\\b)";
        // std::regex variable_regex(variable_pattern);

        // std::string split_pattern = "([a-zA-Z_$][\\w$]*|\\\\\\s|\\s)";
        // std::regex split_regex(split_pattern);

        // std::string endmark_pattern = "(| ...\\\\n\\\" \\) \\),| ...\\\\n\\\\n)";
        // std::regex endmark_regex(endmark_pattern);

        // std::string comment_pattern = "(//.*\\\\\\\\x0a|//.*\\.\\.\\.\\\\n\\\\n|//.*\\.\\.\\.\\\\n\\\" \\) \\),)";
        // std::regex comment_regex(comment_pattern);

        // std::string hexchar_pattern = "((?:\\\\\\\\|\\\\|0)[xX][0-9a-fA-F]{1,8})";
        // std::regex hexchar_regex(hexchar_pattern);

        // std::string space_pattern = "([\\t\\n\\r\\f\\v]+| [ ]+)";
        // std::regex space_regex(space_pattern);

        // std::string clean_space_pattern = "(. [^$\\w]+ .?|[^$\\w]+ .?|.? [^$\\w]+)";
        // std::regex clean_space_regex(clean_space_pattern);
    }

    bool run(const std::string& inputs) {
        if (!clean(inputs)) {
            return false;
        }

        var_list = getVariableList(snippet);

        std::cout << "len of var list is " << var_list.size() << std::endl;
        std::cout << "var list: ['";
        for (std::string var : var_list) {
            std::cout << var << "', '";
            // if (var !== var_list.back()) {
            //     // std::cout << var << "', '";
            // } else {
            //     // std::cout << var << "']";
            // }
        } 
        std::cout << std::endl;

        splitSnippet();

        std::cout << "len of word list is " << words.size() << std::endl;
        std::cout << "word list: ['";
        for (std::string w : words) {
            std::cout << w << "', '";
            // if (w != words.back()) {
            //     std::cout << w << "', '";
            // } else {
            //     // std::cout << w << "']";
            // }
        } 
        std::cout << std::endl;

        checkSnippet();
        replaceWords();

        std::cout << "before wrap: " << code_regex << std::endl;

        wrapParamList();

        std::cout << "before hex char convert: " << code_regex << std::endl;

        wrapHexChar();

        std::cout << code_regex << std::endl;
        
        return true;
    }

private:
    std::string regex_replace_with_lambda(const std::string& input, const std::regex& pattern, const int& group_num, const std::function<std::string(const std::smatch&)>& replacer) {
        // std::cout << "start replacing " << input << std::endl;
        std::string result = input;
        std::smatch match;

        while (std::regex_search(result, match, pattern)) {
            std::string replacement = replacer(match);
            // std::cout << "replacement is: " << replacement << std::endl;
            result = match.prefix().str() + replacement + match.suffix().str();
        }

        return result;
    }

    bool clean(const std::string& inputs) {
        std::string temp_snippet = inputs;
        std::cout << "temp_snippet is: " << temp_snippet << std::endl;
        // 0. clean the comment
        temp_snippet = std::regex_replace(inputs, comment_regex, "");
        std::cout << "0 temp_snippet is: " << temp_snippet << std::endl;
        // 1. convert hex char
        // TODO: handle \\x123 in regex "0x89ff. Then test a hex with capital char 0X89FF. Then test the unusual ones "
        temp_snippet = "\\\\\\\\x89ff, \\\\x89ff and \\x89ff. But this should not be changed x89ff or _0x89ff.";
        temp_snippet = regex_replace_with_lambda(temp_snippet, hexchar_regex, 1, [](const std::smatch& c) {
            // TODO: DCHECK the result 
            std::string hexValue;
            if (c.str(1)[0] == '0') {
                hexValue = c.str(1);
            } else { // handling \\x123
                hexValue = c.str(1);
                hexValue = "0x";
                size_t xPos = c.str().find('x');
                hexValue.append(c.str(1).substr(xPos));
            }
            std::cout << "hexValue is " << hexValue << std::endl;
            std::string result = "";
            if (c.str() != c.str(1)) {
                result += c.str()[0];
            }
            result += std::to_string(std::stoi(hexValue, nullptr, 16));
            return result;
        });
        std::cout << "1 temp_snippet is: " << temp_snippet << std::endl;
        // 2. clean the repeated space characters
        temp_snippet = std::regex_replace(temp_snippet, space_regex, "");
        // TODO: clean redundant backslash
        std::cout << "2 temp_snippet is: " << temp_snippet << std::endl;
        // 3. clean the endmark
        temp_snippet = std::regex_replace(temp_snippet, endmark_regex, "");
        std::cout << "3 temp_snippet is: " << temp_snippet << std::endl;
        // determine whether this snippet contains enough information
        if (temp_snippet.length() <= 34) return false;
        // 4. replace ' and "
        temp_snippet = std::regex_replace(temp_snippet, std::regex("\\\\\""), "\"");
        temp_snippet = std::regex_replace(temp_snippet, std::regex("\\\\'"), "'");
        std::cout << "4 temp_snippet is: " << temp_snippet << std::endl;
        // 5. unescape "\\\\"
        temp_snippet = std::regex_replace(temp_snippet, std::regex("\\\\\\\\"), "\\");
        std::cout << "5 temp_snippet is: " << temp_snippet << std::endl;
        // 6. clean the single space character and semicolon before }
        temp_snippet = second_clean(temp_snippet);
        std::cout << "6 temp_snippet is: " << temp_snippet << std::endl;

        snippet = temp_snippet;

        return true;
    }

    std::vector<std::string> findall(const std::string& text, const std::regex& pattern) {
        std::vector<std::string> results;
        std::sregex_iterator it(text.begin(), text.end(), pattern);
        std::sregex_iterator end;
        std::cout << "Findall text is: " << text << std::endl; 
        for (; it != end; ++it) {
            std::cout << "iter now is: " << it->str() << std::endl; 
            results.push_back(it->str());
        }
        return results;
    }

    std::string second_clean(const std::string& input) {
        // std::string temp_snippet = input;
        // std::vector<std::string> strings;
        // std::regex string_regex(R"(((?<!\\)'.*?(?<!\\)'|(?<!\\)".*?(?<!\\)"|(?<!\\)`.*?(?<!\\)`|(?<!\\)/.*?(?<!\\)/[dgimsuy]?|(?:'|"|\`|/).*\Z))");

        strings = findall(input, string_regex);
        std::cout << "strings.size() is: " << strings.size() << std::endl;

        std::string cleaned = "";

        std::sregex_token_iterator iter(input.begin(), input.end(), string_regex, -1);
        std::sregex_token_iterator end;
        size_t counter = 0;
        for ( ; iter != end; ++iter) {
            std::cout << "substr is: " << *iter << std::endl;
            std::string tmp = regex_replace_with_lambda(*iter, clean_space_regex, 1, [](const std::smatch& obj) {
                std::cout << "matched: " << obj.str() << std::endl;
                return std::regex_replace(obj.str(), std::regex(" "), "");
            });
            cleaned += tmp;
            cleaned = std::regex_replace(cleaned, std::regex(";\\}"), "}");
            if (counter != strings.size()) {
                cleaned += strings.at(counter++);
            }
        }
        
        return cleaned;
    }

    std::vector<std::string> getVariableList(const std::string &snippet) {
        std::vector<std::string> varList;
        // std::regex variable_regex(R"([^a-zA-Z0-9_$]([a-zA-Z_$][\w$]*)\b)");
        // std::cout << "snippet is: " << snippet << std::endl;

        std::sregex_iterator iter(snippet.begin(), snippet.end(), variable_regex);
        std::sregex_iterator end;

        while (iter != end) {
            std::string varname = (*iter)[1];
            if (std::find(keywords.begin(), keywords.end(), varname) == keywords.end()) {
                varList.push_back((*iter)[1]);
                // std::cout << "varname found is: " << (*iter)[1] << "(end)" << std::endl;
            }
            ++iter;
        }

        return varList;
    }

    void splitSnippet() {
        std::sregex_token_iterator iter(this->snippet.begin(), this->snippet.end(), string_regex, -1);
        std::sregex_token_iterator end;
        size_t counter = 0;

        std::string input = snippet;

        std::vector<std::string> temp_words;
        for ( ; iter != end; ++iter) {
            std::string part = *iter;
            std::smatch match;
            while (std::regex_search(part, match, split_regex)) {
                if (match.prefix().str() != "") {
                    temp_words.push_back(match.prefix().str());
                }
                temp_words.push_back(match.str());
                part = match.suffix().str();
            }
            if (part != "") {
                temp_words.push_back(part);
            }
            if (counter != strings.size()) {
                temp_words.push_back(strings.at(counter++));
            }
        }
        words = temp_words;      
    }

    void checkSnippet() {
        // Add a pseudo-name to the snippet that does not have a function name
        if (words.size() > 1 && words[1].compare(0, 1, "(") == 0) {
            words.insert(words.begin() + 1, " ");
            words.insert(words.begin() + 2, "functionName");
            var_list.push_back("functionName");
        }

        // Delete the last word if it is incomplete
        if (std::find(var_list.begin(), var_list.end(), words.back()) != var_list.end() &&
            std::find(keywords.begin(), keywords.end(), words.back()) == keywords.end()) {
            words.pop_back();
        }
    }

    std::string escapeRegex(const std::string &input) {
        // Characters that need to be escaped in a regex
        const std::string specialChars = "()[]{}?*+-|^$\\.&~# \t\n\r\v\f";

        std::string escaped;
        for (char c : input) {
            if (specialChars.find(c) != std::string::npos) {
                escaped += '\\';
            }
            escaped += c;
        }

        return escaped;
    }

    void replaceWords() {
        // Construct the code_regex by replacing words
        std::string code_regex_str = "";
        for (std::string word : words) {
            if (std::find(strings.begin(), strings.end(), word) != strings.end()) {
                // the word is in self.strings
                if (word[0] == '`' || word[0] == '/') {
                    code_regex_str += escapeRegex(word);
                    continue;
                }
                if (word.size() == 1 || (word[word.size() - 1] != '\'' && word[word.size() - 1] != '"')) {
                    word = word.substr(1);
                    code_regex_str += "('|\")" + escapeRegex(word);
                } else {
                    word = word.substr(1, word.size() - 2);
                    code_regex_str += "('|\")" + escapeRegex(word) + "('|\")";
                }
            } else if (std::find(keywords.begin(), keywords.end(), word) != keywords.end() || 
                       std::find(func_names.begin(), func_names.end(), word) != func_names.end()) {
                // the word is a keyword or func_name
                code_regex_str += escapeRegex(word);
            } else if (std::find(var_list.begin(), var_list.end(), word) == var_list.end()) {
                // the word is not a var
                code_regex_str += escapeRegex(word);
            } else {
                code_regex_str += "[a-zA-Z_$][\\w$]*?";
            }
        }
        code_regex = code_regex_str;
    }

    void wrapParamList() {
        std::string input = code_regex; 
        std::string from = "\\ [a-zA-Z_$][\\w$]*?";
        std::string to ="(\\ [a-zA-Z_$][\\w$]*?)?";
        size_t start_pos = input.find(from);
        if(start_pos == std::string::npos)
            code_regex = input;
        input.replace(start_pos, from.size(), to);
        code_regex = input;
    }

    void wrapHexChar() {
        std::string input = code_regex;
        std::regex numberPattern("([0-9]+)");
        std::sregex_iterator iterator(input.begin(), input.end(), numberPattern);
        std::sregex_iterator end;
        std::string output = input;

        // Track the difference in length due to replacements
        int lengthDifference = 0;

        // Iterate through matches and replace them with hexadecimal equivalents
        while (iterator != end) {
            std::smatch match = *iterator;

            std::stringstream result;
            result << "(?:" << match.str() << "|" << "0x" << std::hex << stoi(match.str()) << ")";

            size_t startPos = match.position() + lengthDifference;
            size_t length = match.length();
            output.replace(startPos, length, result.str());  // Replace the number with its hexadecimal equivalent
            
            // Update the length difference
            int lengthChange = result.str().length() - length;
            lengthDifference += lengthChange;

            iterator++;
        }
        code_regex = output;
    }

public:
    std::string getCodeRegex() const {
        return code_regex;
    }
};

// }; // end of namespace internal
// }; // end of namespace v8

std::vector<std::string> findall(const std::string& text, const std::regex& pattern) {
    std::vector<std::string> results;
    std::sregex_iterator it(text.begin(), text.end(), pattern);
    std::sregex_iterator end;
    std::cout << "Findall text is: " << text << std::endl; 
    for (; it != end; ++it) {
        std::cout << "iter now is: " << it->str() << std::endl; 
        results.push_back(it->str());
    }
    return results;
}

int main() {
    // v8::internal::CodeRegexExtractor cre;
    CodeRegexExtractor cre;
    std::string s = "function $t(_0x5a0cb2){var _0xde0fc6=_0x31c04c>-0x1?_0x5a0cb2[_0x45ec('0x56')](0x0,_0x31c04c):_0x5a0cb2;var _0x8f1dd5=_0x31c04c>-0x1?_0x18b508(_0x5a0cb2[";
    // std::string s = "function getAllUrlParams(url) {\\\\x0a\\\\x09\\\\x09    var queryString = url ? url.split(\\'?\\')[1] : window.location.search.slice(1);\\\\x0a\\\\x09\\\\x09    var obj = {};\\\\x0a\\\\x09\\\\x09    if (queryString) {\\\\x0a\\\\x09\\\\x09      queryString = queryString.split(\\'#\\')[0];\\\\x0a\\\\x09\\\\x09      var arr = queryString.split(\\'&\\');\\\\x0a\\\\x09\\\\x09      for (var i=0; i<arr.length; i++) {\\\\x0a\\\\x09\\\\x09        var a = arr[i].split...\\n\\n";
    if (cre.run(s)) {
        // std::string regex = cre.getCodeRegex();
        // std::cout << "regex is: " << regex << std::endl;
        // std::cout << "substr is: " << s << std::endl;
        std::cout << "finish" << std::endl;
    }
    // std::string string_pattern = "('.*?'|\".*?\"|\\\".*?\\\"|`.*?`|/.*?/[dgimsuy]?|(?:'|\\\"|`|/).*\\Z)";
    // std::regex string_regex(string_pattern);
    // std::vector<std::string> strings = findall(s, string_regex);
    // for (const std::string& substr : strings) {
    //     std::cout << "substr is: " << substr << std::endl;
    // }
    

    return 0;
}
