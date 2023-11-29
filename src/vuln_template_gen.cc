#include <iostream>
#include <string>
#include <vector>
#include <regex>

// Helper functions from utils.py
std::vector<std::string> endmark_list = {
    "\\n",
    "...\\n\" ) ),",
    "...\\n\\n"
};

std::regex endmark_regex;
std::regex comment_regex;
std::regex hexchar_regex;
std::regex space_regex;
std::regex string_regex;
std::regex clean_space_regex;

void initialize_regex() {
    std::string endmark_pattern = "(";
    for (const auto& endmark : endmark_list) {
        endmark_pattern += std::regex_escape(endmark) + "$|";
    }
    endmark_pattern.pop_back(); // Remove the last '|'
    endmark_pattern += ")";
    endmark_regex = std::regex(endmark_pattern);

    comment_regex = std::regex(R"(//.*\\x0a|//.*\.\.\.\\n\\n|//.*\.\.\.\\n\" \) \),)");
    hexchar_regex = std::regex(R"(\\x[0-9a-z]{2})");
    space_regex = std::regex(R"([\t\n\r\f\v]+| [ ]+)");
    string_regex = std::regex(R"((?<!\\)'.*?(?<!\\)'|(?<!\\)\".*?(?<!\\)\"|(?<!\\)`.*?(?<!\\)`|(?<!\\)/.*?(?<!\\)/[dgimsuy]?|(?:'|"|\`|/).*\Z)");
    clean_space_regex = std::regex(R"((. [^$\w]+ .?|[^$\w]+ .?|.? [^$\w]+))");
}

std::string _second_clean(const std::string& snippet) {
    std::vector<std::string> strings;
    std::smatch match;
    std::string cleaned = "";
    std::string substr = snippet;
    while (std::regex_search(substr, match, string_regex)) {
        strings.push_back(match[0]);
        substr = match.prefix().str() + match.suffix().str();
    }

    for (const auto& str : strings) {
        cleaned += str;
    }

    substr = snippet;
    while (std::regex_search(substr, match, string_regex)) {
        substr = match.prefix().str();
        std::string tmp = std::regex_replace(substr, clean_space_regex, [](const std::smatch& obj) {
            return obj.str().find_first_of(" \t") == std::string::npos ? obj.str() : obj.str().substr(0, 1);
        });
        cleaned += tmp.find(";}") != std::string::npos ? tmp.replace(";}", "}") : tmp;
        cleaned += match[0];
        substr = match.suffix().str();
    }

    return cleaned;
}

std::string cleaner(const std::string& snippet) {
    std::string cleaned = snippet;
    // 0. clean the comment
    cleaned = std::regex_replace(cleaned, comment_regex, "");
    // 1. convert hex char
    cleaned = std::regex_replace(cleaned, hexchar_regex, [](const std::smatch& c) {
        return std::string(1, std::stoi(c.str().substr(2), nullptr, 16));
    });
    // 2. clean the repeated space characters
    cleaned = std::regex_replace(cleaned, space_regex, "");
    // 3. clean the endmark
    cleaned = std::regex_replace(cleaned, endmark_regex, "");
    // 4. replace ' and "
    cleaned = std::regex_replace(cleaned, std::regex("\\\\\""), "\"");
    cleaned = std::regex_replace(cleaned, std::regex("\\\\'"), "'");
    // 5. unescape "\\\\"
    cleaned = std::regex_replace(cleaned, std::regex("\\\\\\\\"), "\\");
    // 6. clean the single space and semicolon before '}'
    cleaned = _second_clean(cleaned);
    return cleaned;
}

// Helper functions from extract_regex_v2.py
std::regex parentheses(R"((?<!\\)(\(|\)))");
std::regex re_string_regex(R"(((?<!\\)\'\|\"\).*?(?<!\\)\'\|\"\)|(?<!\\)`.*?(?<!\\)`|(?<!\\)/.*?(?<!\\)/[dgimsuy]?|(?:\(\'\|\"\)|\`|/).*\Z))");

std::vector<std::pair<int, int>> parse_parenthesis(const std::string& regex) {
    std::vector<std::pair<int, int>> idx;
    std::string retmp(1, regex[0]);
    int i = 1;
    std::vector<int> stk;
    int beg = 0;
    if (regex[0] == '(') stk.push_back(0);
    while (i < regex.length()) {
        char cur = regex[i];
        char pre = regex[i - 1];
        retmp += cur;
        if (cur == '(' && pre != '\\') {
            stk.push_back(i);
        } else if (cur == ')' && pre != '\\') {
            beg = stk.back();
            if (i + 1 < regex.length() && regex[i + 1] == '?') {
                int end = i + 1;
                retmp += '?';
                i += 1;
                idx.emplace_back(beg, end + 1);
            } else {
                int end = i;
                idx.emplace_back(beg, end + 1);
            }
            stk.pop_back();
        }
        i += 1;
    }
    return idx;
}

std::vector<std::pair<int, int>> parse_var_regex(const std::string& regex, int start) {
    std::regex var_regex(R"(([\w$]+))");
    std::sregex_iterator it(regex.begin(), regex.end(), var_regex);
    std::sregex_iterator end;
    std::vector<std::pair<int, int>> var_idx;
    int cur_len = 0;
    for (; it != end; ++it) {
        std::smatch match = *it;
        std::string part = match.str();
        var_idx.emplace_back(start + cur_len, start + cur_len + part.length());
        cur_len += part.length();
    }
    return var_idx;
}

std::vector<std::pair<int, int>> parse_strings(const std::string& regex, int start) {
    std::vector<std::pair<int, int>> str_idx;
    std::smatch match;
    std::string substr = regex;
    while (std::regex_search(substr, match, re_string_regex)) {
        std::string part = match[0].str();
        str_idx.emplace_back(start + regex.find(substr), start + regex.find(substr) + part.length());
        substr = match.prefix().str() + match.suffix().str();
    }
    return str_idx;
}

std::vector<std::pair<std::pair<int, int>, std::string>> check_parse_regex(const std::string& regex, const std::vector<std::pair<int, int>>& idx) {
    std::vector<std::pair<std::pair<int, int>, std::string>> idx_res;
    for (const auto& item : idx) {
        idx_res.emplace_back(std::make_pair(item, item.second - item.first, regex.substr(item.first, item.second - item.first)));
    }
    return idx_res;
}

std::vector<std::pair<std::pair<int, int>, int>> get_matched_obj(const std::string& a, const std::string& b, const std::vector<std::pair<int, int>>& i_idx, const std::vector<std::pair<int, int>>& j_idx, int a_start = 0, int b_start = 0) {
    std::vector<std::pair<std::pair<int, int>, int>> final_obj;
    std::smatch match;
    std::string matched;
    std::vector<std::pair<std::pair<int, int>, int>> match_obj;
    std::string::const_iterator a_it = a.begin();
    std::string::const_iterator b_it = b.begin();
    std::regex rx = std::regex("(" + std::regex_escape(a_start == 0 ? "(" : " ") + ")");
    while (std::regex_search(a_it, a.end(), match, rx) && std::regex_search(b_it, b.end(), match, rx)) {
        int i = match.position(1);
        int j = match.position(1);
        int size = match.length(1);
        matched = match.str(1);
        if (size == 0) {
            final_obj.emplace_back(std::make_pair(a_start + i, b_start + j), size);
            return final_obj;
        }
        if (in_regex(a_start + i, size, i_idx) || in_regex(b_start + j, size, j_idx)) {
            a_it += i + size;
            b_it += j + size;
            continue;
        }
        if (matched == "\\") {
            a_it += i + 1;
            b_it += j + 1;
            continue;
        }
        if (matched.back() == '\\' && !(i + size == a.length() || j + size == b.length())) {
            matched.pop_back();
            size -= 1;
            while (size > 0 && matched.back() == '\\') {
                matched.pop_back();
                size -= 1;
            }
            if (size > 2) {
                final_obj.emplace_back(std::make_pair(a_start + i, b_start + j), size);
            }
            std::vector<std::pair<std::pair<int, int>, int>> obj = get_matched_obj(a.substr(i + size), b.substr(j + size), i_idx, j_idx, i + size + a_start, j + size + b_start);
            final_obj.insert(final_obj.end(), obj.begin(), obj.end());
            break;
        }
        if (size > 2) {
            final_obj.emplace_back(std::make_pair(a_start + i, b_start + j), size);
        }
        a_it += i + size;
        b_it += j + size;
    }
    return final_obj;
}

bool in_regex(int idx, int size, const std::vector<std::pair<int, int>>& var_idx) {
    std::set<int> cur_set;
    for (int i = idx; i < idx + size; ++i) {
        cur_set.insert(i);
    }
    for (const auto& item : var_idx) {
        int beg = item.first;
        int end = item.second;
        std::set<int> tmp_set;
        for (int i = beg; i < end; ++i) {
            tmp_set.insert(i);
        }
        std::set<int> same;
        std::set_intersection(cur_set.begin(), cur_set.end(), tmp_set.begin(), tmp_set.end(), std::inserter(same, same.begin()));
        if (!same.empty() && same != tmp_set) {
            return true;
        }
    }
    return false;
}

std::pair<std::string, int> middle_merge(const std::string& tmp_a, const std::string& tmp_b) {
    int idx_a = tmp_a.length();
    int idx_b = tmp_b.length();
    int min_len = std::min(idx_a, idx_b);
    std::string same = "";
    int i = 0;
    while (i < min_len) {
        if (tmp_a[idx_a - i - 1] != tmp_b[idx_b - i - 1]) {
            break;
        }
        char cur = tmp_a[idx_a - i - 1];
        if (cur == '(' || cur == ')' || cur == '?') {
            break;
        }
        same = cur + same;
        i += 1;
    }
    return std::make_pair(same, i);
}

std::string _merge_helper(const std::string& tmp_a, const std::string& tmp_b) {
    if (!tmp_a.empty() && !tmp_b.empty()) {
        std::vector<std::string> a_enum_regex = _enumerate_regex(tmp_a);
        std::vector<std::string> b_enum_regex = _enumerate_regex(tmp_b);
        if (std::find(a_enum_regex.begin(), a_enum_regex.end(), tmp_b) != a_enum_regex.end()) return tmp_a;
        if (std::find(b_enum_regex.begin(), b_enum_regex.end(), tmp_a) != b_enum_regex.end()) return tmp_b;

        if (!_repeated_wrap(tmp_a)) tmp_a = "(" + tmp_a + ")";
        if (!_repeated_wrap(tmp_b)) tmp_b = "(" + tmp_b + ")";
        return "(" + tmp_a + "|" + tmp_b + ")";
    }
    if (!tmp_a.empty()) {
        return _repeated_wrap(tmp_a) ? tmp_a : tmp_a + "?";
    }
    if (!tmp_b.empty()) {
        return _repeated_wrap(tmp_b) ? tmp_b : tmp_b + "?";
    }
    return "";
}

std::vector<std::string> _enumerate_regex(const std::string& regex) {
    std::vector<std::string> enum_lst;
    if (!_repeated_wrap(regex)) {
        return enum_lst;
    }

    if (regex.back() == ')') {
        regex.substr(1, regex.length() - 2);
    } else if (regex.length() > 2 && regex.substr(regex.length() - 2) == ")?") {
        regex.substr(1, regex.length() - 3);
        enum_lst.push_back("");
    }

    std::vector<std::string> op_stk;
    std::vector<std::string> vl_stk;
    if (regex[0] == '(') op_stk.push_back("(");

    int i = 1;
    std::string tmp;
    while (i < regex.length()) {
        char cur = regex[i];
        char pre = regex[i - 1];
        if (cur == '(' && pre != '\\') {
            if (!op_stk.empty() && op_stk.back() == "?") {
                vl_stk = _enumerate_regex_helper(vl_stk, tmp);
                op_stk.pop_back();
            }
            op_stk.push_back("(");
            tmp.clear();
        } else if (cur == ')' && pre != '\\') {
            std::string op = op_stk.back();
            if (op == "?") {
                vl_stk = _enumerate_regex_helper(vl_stk, tmp);
                op_stk.pop_back();
            } else if (op == "(") {
                vl_stk.push_back(tmp);
            }
            tmp.clear();
            op_stk.pop_back();
        } else if (cur == '|' && pre != '\\') {
            if (!op_stk.empty() && op_stk.back() == "?") {
                vl_stk = _enumerate_regex_helper(vl_stk, tmp);
                op_stk.pop_back();
            }
            while (!vl_stk.empty()) {
                enum_lst.push_back(vl_stk.back() + tmp);
                vl_stk.pop_back();
            }
        } else if (cur == '?' && pre == ')') {
            op_stk.push_back("?");
        } else {
            tmp += cur;
        }
        i += 1;
    }

    while (!vl_stk.empty()) {
        enum_lst.push_back(vl_stk.back() + tmp);
        vl_stk.pop_back();
    }

    return enum_lst;
}

std::vector<std::string> _enumerate_regex_helper(const std::vector<std::string>& vl_stk, const std::string& tmp) {
    std::vector<std::string> new_vl_lst;
    if (vl_stk.empty()) {
        new_vl_lst.push_back(tmp);
        new_vl_lst.push_back("");
        return new_vl_lst;
    }

    std::string pre_regex = vl_stk.back();
    std::string append1 = pre_regex + tmp;
    std::string append2 = tmp;
    if (vl_stk.empty()) {
        new_vl_lst.push_back(append1);
        new_vl_lst.push_back(append2);
    }

    while (!vl_stk.empty()) {
        pre_regex = vl_stk.back();
        new_vl_lst.push_back(pre_regex + append1);
        new_vl_lst.push_back(pre_regex + append2);
        vl_stk.pop_back();
    }

    return new_vl_lst;
}

bool _repeated_wrap(const std::string& regex) {
    if (regex[0] != '(') return false;
    std::vector<std::pair<int, int>> idx = parse_parenthesis(regex);
    for (const auto& item : idx) {
        if (regex.substr(item.first, item.second - item.first) == regex) {
            return true;
        }
    }
    return false;
}

// Helper functions from regex_diff.py
double is_similar(const std::string& a, const std::string& b, double threshold = 0.85, bool strict = true) {
    std::smatch s;
    std::regex_match(a, b, s);
    double score0 = static_cast<double>(s.size()) / std::min(a.length(), b.length());

    if (strict) {
        return score0 >= threshold && std::min(a.length(), b.length()) > 205;
    }

    int min_len = std::min(a.length(), b.length());
    std::string a_substr = a.substr(0, min_len);
    std::string b_substr = b.substr(0, min_len);
    std::regex_match(a_substr, b_substr, s);
    double score1 = static_cast<double>(s.size()) / min_len;

    return score0 >= threshold || score1 >= threshold;
}

double get_score(const std::string& a, const std::string& b) {
    std::smatch s;
    std::regex_match(a, b, s);
    double score0 = static_cast<double>(s.size()) / std::min(a.length(), b.length());

    int min_len = std::min(a.length(), b.length());
    std::string a_substr = a.substr(0, min_len);
    std::string b_substr = b.substr(0, min_len);
    std::regex_match(a_substr, b_substr, s);
    double score1 = static_cast<double>(s.size()) / min_len;

    return std::max(score0, score1);
}

std::string merge_regex(const std::string& a, const std::string& b) {
    std::vector<std::pair<int, int>> a_re_idx = parse_regex(a);
    std::vector<std::pair<int, int>> b_re_idx = parse_regex(b);

    std::vector<std::pair<std::pair<int, int>, int>> match_obj = get_matched_obj(a, b, a_re_idx, b_re_idx);

    std::string new_regex;
    int pre_i = 0, pre_j = 0;
    std::string tmp_a, tmp_b;
    for (const auto& item : match_obj) {
        int i = item.first.first;
        int j = item.first.second;
        int size = item.second;
        std::string ori = a.substr(i, size);
        if (size == 0) {
            tmp_a += a.substr(pre_i, i - pre_i);
            tmp_b += b.substr(pre_j, j - pre_j);
            new_regex += _merge_helper(tmp_a, tmp_b);
        } else if (size > 6) {
            if (pre_i != i) tmp_a += a.substr(pre_i, i - pre_i);
            if (pre_j != j) tmp_b += b.substr(pre_j, j - pre_j);
            std::pair<std::string, int> middle_res = middle_merge(tmp_a, tmp_b);
            std::string middle_same = middle_res.first;
            int middle_idx = middle_res.second;
            tmp_a = tmp_a.substr(0, tmp_a.length() - middle_idx);
            tmp_b = tmp_b.substr(0, tmp_b.length() - middle_idx);
            new_regex += _merge_helper(tmp_a, tmp_b);
            new_regex += (middle_same + ori);
            pre_i = i + size;
            pre_j = j + size;
            tmp_a.clear();
            tmp_b.clear();
        }
    }

    return new_regex;
}

std::vector<std::pair<int, int>> parse_regex(const std::string& regex) {
    std::vector<std::pair<int, int>> idx_res = parse_parenthesis(regex);
    std::vector<std::pair<int, int>> prth_idx;
    for (const auto& item : idx_res) {
        if (item.second != static_cast<int>(regex.find("(?:"))) {
            prth_idx.push_back(item);
        }
    }

    std::vector<std::pair<int, int>> idx;
    int start = 0;
    for (const auto& prth : prth_idx) {
        std::vector<std::pair<int, int>> ret = parse_var_regex(regex.substr(start, prth.first - start), start);
        idx.insert(idx.end(), ret.begin(), ret.end());
        ret = parse_strings(regex.substr(start, prth.first - start), start);
        idx.insert(idx.end(), ret.begin(), ret.end());
        idx.push_back(prth);
        start = prth.second;
    }
    std::vector<std::pair<int, int>> ret = parse_var_regex(regex.substr(start, regex.length() - start), start);
    idx.insert(idx.end(), ret.begin(), ret.end());
    ret = parse_strings(regex.substr(start, regex.length() - start), start);
    idx.insert(idx.end(), ret.begin(), ret.end());

    return idx;
}

int main() {
    // Initialize regex patterns from utils.py
    initialize_regex();

    // Sample usage of vuln_regex_match
    std::string regex_a = "((file\\\\:\\\\/\\\\\\\\{2,3})|(http[s]?)|(ftp[s]?)|(ssh\\\\d?)|(md4|md5|sha1|sha|sha256|sha384|sha512|ssha|smd5|blowfish|bcrypt))";
    std::string regex_b = "(md5|sha1|sha|sha256|sha384|sha512|ssha|smd5|blowfish|bcrypt)";

    std::cout << "Regex A: " << cleaner(regex_a) << std::endl;
    std::cout << "Regex B: " << cleaner(regex_b) << std::endl;

    std::string merged_regex = merge_regex(regex_a, regex_b);
    std::cout << "Merged Regex: " << cleaner(merged_regex) << std::endl;

    return 0;
}
