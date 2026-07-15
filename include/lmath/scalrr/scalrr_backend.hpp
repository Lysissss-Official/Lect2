#ifndef SCALRR_BACKEND_HPP
#define SCALRR_BACKEND_HPP

#define SCALRR_VERSION "26.0.13"
#define SCALRR_VERSION_MAJOR 26
#define SCALRR_VERSION_MINOR 0
#define SCALRR_VERSION_PATCH 13

#include <ostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <iterator>

// Scal辅助部分
namespace scal {
    inline uint64_t max(uint64_t a, uint64_t b) {return a > b ? a : b;}
    inline uint32_t max(uint32_t a, uint32_t b) {return a > b ? a : b;}
    inline uint16_t max(uint16_t a, uint16_t b) {return a > b ? a : b;}
    inline uint8_t max(uint8_t a, uint8_t b) {return a > b ? a : b;}
    inline int64_t max(int64_t a, int64_t b) {return a > b ? a : b;}
    inline int32_t max(int32_t a, int32_t b) {return a > b ? a : b;}
    inline int16_t max(int16_t a, int16_t b) {return a > b ? a : b;}
    inline int8_t max(int8_t a, int8_t b) {return a > b ? a : b;}

    inline uint64_t min(uint64_t a, uint64_t b) {return a < b ? a : b;}
    inline uint32_t min(uint32_t a, uint32_t b) {return a < b ? a : b;}
    inline uint16_t min(uint16_t a, uint16_t b) {return a < b ? a : b;}
    inline uint8_t min(uint8_t a, uint8_t b) {return a < b ? a : b;}
    inline int64_t min(int64_t a, int64_t b) {return a < b ? a : b;}
    inline int32_t min(int32_t a, int32_t b) {return a < b ? a : b;}
    inline int16_t min(int16_t a, int16_t b) {return a < b ? a : b;}
    inline int8_t min(int8_t a, int8_t b) {return a < b ? a : b;}

    // 10的自然数幂
    constexpr std::uint32_t pow_10[10] = {
        1U, 10U, 100U, 1000U, 10000U,
        100000U, 1000000U, 10000000U, 100000000U, 1000000000U
    };
}

// 基本拓展 integer 型 (整数)
// 理论上这里还有一个class

// 基本双拓展 integer 型 (分数)
class Fractional {
    ;
};

// 基本拓展 double 型 (小数)
class Unbounded {
    public:
        std::vector<std::uint32_t> number{};  //  数据存储 (大端模式 Big Endian, 以 10^9 为基数压位, 单元素存9位十进制数)
        std::uint64_t ipart_64b = 0;    //  数据整数部分占用元素数
        std::uint64_t fpart_64b = 0;    //  数据分数部分占用元素数
        bool sign = false;          //  正负符号标记 (为负数为真)

        // 废除 bool type = false;          //  虚实类型标记 (为虚数为真)

        // 废除 std::int64_t exponent;      //  科学计数法指数 (为 10^exponent)
        // 改用:
        int64_t getExponent() const {
            if (ipart_64b > 0) {
                uint64_t digits = 0;
                uint64_t temp = number[0];
                while (temp > 0) {
                    temp /= 10;
                    ++digits;
                }
                return digits + (ipart_64b - 1) * 9 - 1;
            }
            else if (fpart_64b > 0) {
                for (auto i = ipart_64b; i < number.size(); i++) {
                    if (number[i] != 0) {
                        uint64_t digits = 0;
                        uint64_t temp = number[i];
                        while (temp > 0) {
                            temp /= 10;
                            ++digits;
                        }
                        return 0 - (i - ipart_64b + 1) * 9 + digits - 1;
                    }
                }
            }
            else {
                return 0;
            }
            return 0;
        }
        uint64_t getDigit(int64_t exp) const {

            if (exp < 0) {
                exp = -exp;
                uint64_t keep_full = (exp-1) / 9;
                uint64_t keep_bits = (exp-1) % 9;
                uint64_t length = keep_full + 1;
                if (length <= fpart_64b) {
                    return number[ipart_64b + length - 1] % scal::pow_10[8-keep_bits+1] / scal::pow_10[8-keep_bits];
                }
                else {
                    throw std::runtime_error("Scal: Invalid exponent");
                }
            }
            else {
                uint64_t keep_full = exp / 9;
                uint64_t keep_bits = exp % 9;
                uint64_t length = keep_full + 1;
                if (length <= ipart_64b) {
                    return number[ipart_64b - length] % scal::pow_10[keep_bits+1] / scal::pow_10[keep_bits];
                }
                else {
                    throw std::runtime_error("Scal: Invalid exponent");
                }
            }
        }

    private:
        static void trimLeadingZeros(Unbounded& opt) {
            uint64_t cnt = 0;
            for (int64_t i = 0; i <= (int64_t) opt.ipart_64b - 1; i++) {
                if (opt.number[i] == 0) {
                    cnt++;
                }
                else {
                    break;
                }
            }
            if (cnt != 0) {
                opt.number.erase(opt.number.begin(), opt.number.begin()+cnt);
                opt.ipart_64b -= cnt;
            }
        }
        static void trimTrailingZeros(Unbounded& opt) {
            uint64_t cnt = 0;
            for (int64_t i = (int64_t) opt.number.size() - 1; i >= (int64_t) opt.ipart_64b ; i--) {
                if (opt.number[i] == 0) {
                    cnt++;
                }
                else {
                    break;
                }
            }
            if (cnt != 0) {
                opt.number.erase(opt.number.end()-cnt, opt.number.end());
                opt.fpart_64b -= cnt;
            }
        }
        static bool hqIsZero(const Unbounded& opt) {
            for (std::uint32_t block : opt.number) {
                if (block != 0) return false;
            }
            return true;
        }
        static Unbounded hqAbs(const Unbounded& opt) {
            return {opt.number, opt.ipart_64b, opt.fpart_64b, false};
        }
        static Unbounded hqNeg(const Unbounded& opt) {
            return {opt.number, opt.ipart_64b, opt.fpart_64b, hqIsZero(opt) ? false : !opt.sign};
        }
        // 高精度加法
        static Unbounded hqAdd(const Unbounded& opt1, const Unbounded& opt2) {
            /* --- 转换为同号计算 --- */
            if (opt1.sign ^ opt2.sign) {
                if (opt1.sign) return hqSub(opt2, hqAbs(opt1));
                if (opt2.sign) return hqSub(opt1, hqAbs(opt2));
            }

            uint64_t ipart_max = scal::max(opt1.ipart_64b, opt2.ipart_64b); //  结果的整数部分 vector 元素数
            uint64_t fpart_max = scal::max(opt1.fpart_64b, opt2.fpart_64b); //  结果的小数部分 vector 元素数

            Unbounded result;
            result.sign = opt1.sign;    //  此处已经转为同号计算 opt1.sign == opt2.sign
            result.ipart_64b = ipart_max + 1;
            result.fpart_64b = fpart_max;
            //result.exponent = scal::max(opt1.exponent, opt2.exponent);
            result.number.resize(ipart_max + fpart_max + 1);

            std::uint64_t calc_tmp = 0;    //  压位计算结果缓存
            std::uint64_t carry_tmp = 0;   //  压位计算进位缓存

            /* --- 计算 --- */

            for (int64_t i1 = opt1.ipart_64b+fpart_max-1, i2 = opt2.ipart_64b+fpart_max-1, cnt = ipart_max+fpart_max-1; cnt>=0; i1--, i2--, cnt--) {
                calc_tmp = carry_tmp + (i1>=0 && i1 < static_cast<std::int64_t>(opt1.ipart_64b + opt1.fpart_64b)? opt1.number[i1] :0) + (i2>=0 && i2 < static_cast<std::int64_t>(opt2.ipart_64b + opt2.fpart_64b)? opt2.number[i2] :0);
                carry_tmp = calc_tmp >= 1000000000ULL ? 1 : 0; //  = calc_tmp / (10^9)
                result.number[cnt+1] = calc_tmp - (carry_tmp * 1000000000ULL);
            }
            if (carry_tmp != 0) {
                result.number[0] = carry_tmp;
            }
            //std::cerr << "0k!" << std::endl;
            trimLeadingZeros(result);
            trimTrailingZeros(result);
            if (hqIsZero(result)) result.sign = false;

            return result;
        }
        // 高精度减法
        static Unbounded hqSub(const Unbounded& opt1, const Unbounded& opt2) {
            if (opt1.sign ^ opt2.sign) {
                if (opt1.sign) return hqNeg(hqAdd(hqAbs(opt1), opt2));
                if (opt2.sign) return hqAdd(opt1, hqAbs(opt2));
            }
            if (hqAbsComp(opt2, opt1)) {
                return hqNeg(hqSub(opt2, opt1));
            }

            uint64_t ipart_max = scal::max(opt1.ipart_64b, opt2.ipart_64b);
            uint64_t fpart_max = scal::max(opt1.fpart_64b, opt2.fpart_64b);

            Unbounded result;
            result.sign = opt1.sign;    //  此处已经转为同号计算 opt1.sign == opt2.sign 且 opt1 > opt2
            result.ipart_64b = ipart_max;
            result.fpart_64b = fpart_max;
            //result.exponent = scal::max(opt1.exponent, opt2.exponent);
            result.number.resize(ipart_max + fpart_max);

            std::int64_t calc_tmp = 0;    //  压位计算结果缓存 (自带负号)
            std::int64_t carry_tmp = 0;   //  压位计算借位缓存 (自带负号)

            for (int64_t i1 = opt1.ipart_64b+fpart_max-1, i2 = opt2.ipart_64b+fpart_max-1, cnt = ipart_max+fpart_max-1; cnt>=0; i1--, i2--, cnt--) {
                calc_tmp = carry_tmp + (i1>=0 && i1 < static_cast<std::int64_t>(opt1.ipart_64b + opt1.fpart_64b)? opt1.number[i1] :0) - (i2>=0 && i2 < static_cast<std::int64_t>(opt2.ipart_64b + opt2.fpart_64b)? opt2.number[i2] :0);
                carry_tmp = calc_tmp >= 0 ? 0 : -1;    // = calc_tmp / (10^9)
                result.number[cnt] = calc_tmp - (carry_tmp * 1000000000ULL);
            }
            //std::cerr << "1k!" << std::endl;
            trimLeadingZeros(result);
            trimTrailingZeros(result);
            if (hqIsZero(result)) result.sign = false;

            return result;
        }
        // 高精度乘法
        static Unbounded hqMul(const Unbounded& opt1, const Unbounded& opt2) {
            uint64_t length1 = opt1.ipart_64b + opt1.fpart_64b;
            uint64_t length2 = opt2.ipart_64b + opt2.fpart_64b;
            uint64_t ipart = opt1.ipart_64b + opt2.ipart_64b;
            uint64_t fpart = opt1.fpart_64b + opt2.fpart_64b;

            Unbounded result;
            result.sign = opt1.sign ^ opt2.sign;
            result.ipart_64b = ipart;
            result.fpart_64b = fpart;
            //result.exponent = opt1.exponent + opt2.exponent;
            result.number.resize(length1 + length2, 0);

            std::uint64_t calc_tmp = 0;    //  压位计算结果缓存 (自带负号)
            std::uint64_t carry_tmp = 0;   //  压位计算借位缓存 (自带负号)

            for (int64_t i1 = length1-1; i1 >= 0; i1--) {
                carry_tmp = 0;
                for (int64_t i2 = length2-1; i2 >= 0; i2--) {
                    calc_tmp = carry_tmp + (std::uint64_t)opt1.number[i1] * (std::uint64_t)opt2.number[i2] + result.number[i1 + i2 + 1];
                    carry_tmp = calc_tmp / 1000000000ULL;
                    result.number[i1 + i2 + 1] = calc_tmp - (carry_tmp * 1000000000ULL);
                    //std::cerr << "@pos[" << i1+i2+1 << "],  val = " << result.number[i1 + i2 + 1] << std::endl;
                    //std::cerr << "       ( calc = " << (int64_t)(calc_tmp%1000000000ULL) << std::endl;
                    //std::cerr << "       (carry = " << (int64_t)(carry_tmp%1000000000ULL) << std::endl;
                }
                if (carry_tmp != 0) {
                    result.number[i1] += carry_tmp;
                }
            }

            trimLeadingZeros(result);
            trimTrailingZeros(result);
            if (hqIsZero(result)) result.sign = false;

            return result;
        }
        // 高精度除法
        static Unbounded hqDiv(const Unbounded& a, const Unbounded& b) {
            if (hqEqual(b, {{},0,0})) {
                throw std::runtime_error("Division by zero");
            }
            Unbounded res_1_b;
            int64_t exp = -(int64_t)(b.getExponent())-1;
            if (exp < 0) {
                exp = -exp;
                uint64_t keep_full = (exp-1) / 9;
                uint64_t keep_bits = (exp-1) % 9;
                uint64_t length = keep_full + 1;
                res_1_b.number.resize(length);
                res_1_b.number[length-1] = scal::pow_10[8 - keep_bits];
                res_1_b.fpart_64b = length;
            }
            else {
                uint64_t keep_full = exp / 9;
                uint64_t keep_bits = exp % 9;
                uint64_t length = keep_full + 1;
                res_1_b.number.resize(length);
                res_1_b.number[0] = scal::pow_10[keep_bits];
                res_1_b.ipart_64b = length;
            }

            //std::cerr << "??() " << res_1_b << "  " <<res_1_b.number.size()<< std::endl;
            for (auto i=1; i<=25; i++) {
                //std::cerr << "??++ " << hqAdd(res_1_b, res_1_b) << std::endl;
                //std::cerr << "??** " << hqMul(b,hqMul(res_1_b, res_1_b)) << std::endl;

                res_1_b = hqSub(hqAdd(res_1_b, res_1_b), hqMul(b, hqMul(res_1_b, res_1_b)));

                if (res_1_b.fpart_64b > 10) {
                    res_1_b.fpart_64b = 10;
                    res_1_b.number.erase(res_1_b.number.begin() + res_1_b.ipart_64b + 10, res_1_b.number.end());
                }

                //std::cerr << "??== " << res_1_b << std::endl;
                //std::cerr << "0k";
            }
            return hqMul(a, res_1_b);
        }
        // 高精度绝对值大小比较 (opt1绝对值大于opt2绝对值为真)
        static bool hqAbsComp(const Unbounded& opt1, const Unbounded& opt2) {
            if (opt1.ipart_64b > opt2.ipart_64b) {return true;}
            else if (opt1.ipart_64b < opt2.ipart_64b) {return false;}
            for (std::uint64_t i = 0; i < scal::min(opt1.ipart_64b + opt1.fpart_64b, opt2.ipart_64b + opt2.fpart_64b); ++i) {
                if (opt1.number[i] > opt2.number[i]) {return true;}
                else if (opt1.number[i] < opt2.number[i]) {return false;}
            }
            if (opt1.ipart_64b+opt1.fpart_64b > opt2.ipart_64b+opt2.fpart_64b) {return true;}
            else if (opt1.ipart_64b+opt1.fpart_64b < opt2.ipart_64b+opt2.fpart_64b) {return false;}
            return false;
        }
        // 高精度有符号大小比较 (opt1大于opt2为真)
        static bool hqComp(const Unbounded& opt1, const Unbounded& opt2) {
            const bool zero1 = hqIsZero(opt1);
            const bool zero2 = hqIsZero(opt2);
            if (zero1 && zero2) return false;
            if (opt1.sign != opt2.sign) return !opt1.sign;
            return opt1.sign ? hqAbsComp(opt2, opt1) : hqAbsComp(opt1, opt2);
        }
        // 高精度相等判断 (opt等于opt2为真)
        static bool hqEqual(const Unbounded& opt1, const Unbounded& opt2) {
            if (hqIsZero(opt1) && hqIsZero(opt2)) return true;
            if (opt1.sign != opt2.sign) return false;
            if (opt1.ipart_64b != opt2.ipart_64b || opt1.fpart_64b != opt2.fpart_64b) return false;
            return opt1.number == opt2.number;
        }
    public:
        Unbounded operator+(const Unbounded& b) const {
            return hqAdd(*this, b);
        }
        Unbounded operator-(const Unbounded& b) const {
            return hqSub(*this, b);
        }
        Unbounded operator-() const {
            return hqNeg(*this);
        }
        Unbounded operator*(const Unbounded& b) const {
            return hqMul(*this, b);
        }
        Unbounded operator/(const Unbounded& b) const {
            return hqDiv(*this, b);
        }
        bool operator>(const Unbounded& b) const {
            return hqComp(*this, b);
        }
        bool operator<(const Unbounded& b) const {
            return hqComp(b, *this);
        }
        bool operator>=(const Unbounded& b) const {
            return !(*this < b);
        }
        bool operator<=(const Unbounded& b) const {
            return !(*this > b);
        }
        bool operator==(const Unbounded& b) const {
            return hqEqual(*this, b);
        }
        bool operator!=(const Unbounded& b) const {
            return !(*this == b);
        }
        friend std::ostream& operator<<(std::ostream& os, const Unbounded& num) {
            // 输出负号
            if (num.sign) {
                os << '-';
            }

            // 1e9 压位输出格式
            //const uint64_t BASE = 1000000000ULL;

            // 输出整数部分
            bool leading_zero = true;
            for (uint64_t i = 0; i < num.ipart_64b; ++i) {
                uint64_t val = i < num.number.size() ? num.number[i] : 0;

                if (leading_zero) {
                    if (val == 0) continue;
                    leading_zero = false;
                    os << val;
                } else {
                    // 不足 9 位前面补 0
                    char buf[10];
                    snprintf(buf, sizeof(buf), "%09u", (unsigned)val);
                    os << buf;
                }
            }

            // 全零特殊处理
            if (leading_zero) {
                os << '0';
            }

            // 输出小数部分
            if (num.fpart_64b > 0) {
                os << '.';
                for (uint64_t i = num.ipart_64b; i < num.ipart_64b + num.fpart_64b; ++i) {
                    uint64_t val = i < num.number.size() ? num.number[i] : 0;
                    char buf[10];
                    snprintf(buf, sizeof(buf), "%09u", (unsigned)val);
                    os << buf;
                }
            }

            return os;
        }

};

inline Unbounded hqRead(std::string str) {
    if (str.empty()) {
        throw std::invalid_argument("Scal: empty decimal string");
    }

    bool negative = false;
    if (str.front() == '+' || str.front() == '-') {
        negative = str.front() == '-';
        str.erase(str.begin());
    }
    if (str.empty()) {
        throw std::invalid_argument("Scal: invalid decimal string");
    }

    const std::size_t point = str.find('.');
    if (point != std::string::npos && str.find('.', point + 1) != std::string::npos) {
        throw std::invalid_argument("Scal: invalid decimal string");
    }
    for (char ch : str) {
        if (ch != '.' && (ch < '0' || ch > '9')) {
            throw std::invalid_argument("Scal: invalid decimal string");
        }
    }

    std::string integer = point == std::string::npos ? str : str.substr(0, point);
    std::string fraction = point == std::string::npos ? std::string{} : str.substr(point + 1);
    if (integer.empty()) integer = "0";

    const std::size_t first_nonzero = integer.find_first_not_of('0');
    integer = first_nonzero == std::string::npos ? std::string{} : integer.substr(first_nonzero);
    while (!fraction.empty() && fraction.back() == '0') fraction.pop_back();

    Unbounded result;
    result.sign = negative;

    if (!integer.empty()) {
        const std::size_t first_width = integer.size() % 9 == 0 ? 9 : integer.size() % 9;
        std::size_t pos = 0;
        std::size_t width = first_width;
        while (pos < integer.size()) {
            result.number.push_back(static_cast<std::uint32_t>(std::stoul(integer.substr(pos, width))));
            ++result.ipart_64b;
            pos += width;
            width = 9;
        }
    }

    for (std::size_t pos = 0; pos < fraction.size(); pos += 9) {
        std::string block = fraction.substr(pos, 9);
        block.append(9 - block.size(), '0');
        result.number.push_back(static_cast<std::uint32_t>(std::stoul(block)));
        ++result.fpart_64b;
    }

    if (result.number.empty()) {
        result.sign = false;
    }
    return result;
}

// 基本双拓展 double 型 (小数形式复数)
class Complex {
    public:
        Unbounded real;
        Unbounded imag;
    private:
        ;
    public:
        Complex operator+(const Complex& b) const {
            return {this->real + b.real, this->imag + b.imag};
        }
        Complex operator-(const Complex& b) const {
            return {this->real - b.real, this->imag - b.imag};
        }
        Complex operator*(const Complex& b) const {
            return {this->real * b.real - this->imag * b.imag, this->real * b.imag + this->imag * b.real};
        }
        Complex operator/(const Complex& b) const {
            return {(this->real * b.real + this->imag * b.imag)/(b.real * b.real + b.imag * b.imag), (this->imag * b.real - this->real * b.imag) / (b.real * b.real + b.imag * b.imag)};
        }
        Complex getConjugate() const {
            return {real, -imag};
        }
};

// 复数版的hqRead
inline Complex parseComplex(const std::string&) {
    throw std::logic_error("Scal: complex parser is not implemented");
}

// 基本双双拓展 integer 型 (分数形式复数)
class Fracomplex {
    ;
};

// Scal计算部分
namespace scal {
    /* ===== 常数 ===== */
    inline Unbounded pi(uint64_t /*depth*/) {
        // Demo 莱布尼茨公式 收敛极慢
        /*
         *Unbounded result = hqRead("1");
        for (std::uint64_t i = 1; i <= depth; ++i) {
            result = result - hqRead("1")/hqRead(std::to_string(i*4-1)) + hqRead("1")/hqRead(std::to_string(i*4+1));   //  无穷级数计算
        }
        return result * hqRead("4");
        */
        return hqRead("3.141592653589793238462643383279502884");
    }

    inline Unbounded e(uint64_t depth) {
        Unbounded result = hqRead("1");
        Unbounded fact_tmp = hqRead("1");
        for (std::uint64_t i = 1; i <= depth; ++i) {
            fact_tmp = fact_tmp*hqRead(std::to_string(i));
            result = result + hqRead("1") / fact_tmp;   //  无穷级数计算
        }
        return result;
    }

    /* ===== 函数 ===== */
    /* --- 数值计算 --- */
    // 1的十进制位移
    inline Unbounded exp10(int64_t exp) {
        Unbounded result;
        if (exp < 0) {
            exp = -exp;
            uint64_t keep_full = (exp-1) / 9;
            uint64_t keep_bits = (exp-1) % 9;
            uint64_t length = keep_full + 1;
            result.number.resize(length);
            result.number[length-1] = scal::pow_10[8 - keep_bits];
            result.fpart_64b = length;
            return result;
        }
        else {
            uint64_t keep_full = exp / 9;
            uint64_t keep_bits = exp % 9;
            uint64_t length = keep_full + 1;
            result.number.resize(length);
            result.number[0] = scal::pow_10[keep_bits];
            result.ipart_64b = length;
            return result;
        }
    }
    // 四舍五入
    inline Unbounded round(const Unbounded& opt, uint64_t decimal_places) {
        uint64_t keep_full = decimal_places / 9;
        uint64_t keep_bits = decimal_places % 9;

        if (keep_full >= opt.fpart_64b) {
            return opt;
        }

        Unbounded result = opt;

        result.number.erase(result.number.end() - opt.fpart_64b + keep_full + 1, result.number.end());
        result.fpart_64b = keep_full+1;
        if (result.getDigit(-(int64_t)decimal_places-1) >= 5) {
            result.number[result.ipart_64b + result.fpart_64b - 1] = result.number[result.ipart_64b + result.fpart_64b - 1] / scal::pow_10[9-keep_bits] * scal::pow_10[9-keep_bits];
            return result+exp10(-(int64_t)decimal_places);
        }
        else {
            result.number[result.ipart_64b + result.fpart_64b - 1] = result.number[result.ipart_64b + result.fpart_64b - 1] / scal::pow_10[9-keep_bits] * scal::pow_10[9-keep_bits];
            return result;
        }
    }
    inline Unbounded round2(const Unbounded& opt) {
        Unbounded result = opt;
        result.fpart_64b = 0;
        result.number.erase(result.number.end() - opt.fpart_64b, result.number.end());
        return result;
    }
    // 连加

    // 连乘

    /* --- 乘方，开根与对数 --- */
    // 乘方
    inline Unbounded quickPow(Unbounded opt, uint64_t times) {
        if (times == 0) {
            return hqRead("1");
        }
        Unbounded result = opt;
        uint64_t exp = times - 1;
        Unbounded base = opt;
        while (exp > 0) {
            if (exp & 1) {
                result = result * base;
            }
            base = base * base;
            exp >>= 1;
        }
        return result;
    }
    inline Unbounded pow(Unbounded, Unbounded) {
        throw std::logic_error("Scal: non-integer power is not implemented");
    }
    // 二次方根
    inline Unbounded squareRoot(Unbounded opt) {
        if (opt.sign) {
            // 什么时候正式支持复数了再改
            throw std::runtime_error("Sqrt of negative number");
        }
        if (opt == hqRead("0")) {
            return opt;
        }
        int64_t exp = opt.getExponent();
        Unbounded result = exp10(exp/2);

        Unbounded two = hqRead("2");
        for (auto i = 1; i <= 25; i++) {
            // x = (x + a/x) / 2
            result = (result + opt/result) / two;
            if (result.fpart_64b > 10) {
                result.fpart_64b = 10;
                result.number.erase(result.number.begin() + result.ipart_64b + 10, result.number.end());
            }
        }
        return result;
    }
    // 三次方根
    inline Unbounded cubicRoot(Unbounded opt) {
        if (opt.sign) {
            //
        }
        if (opt == hqRead("0")) {
            return opt;
        }
        int64_t exp = opt.getExponent();
        Unbounded result = exp10(exp/3);
        result.sign = opt.sign;

        Unbounded two = hqRead("2");
        Unbounded three = hqRead("3");
        for (auto i = 1; i <= 25; i++) {
            // x = (2*x + a/x^2) / 3
            result = (two*result + opt/(result*result)) / three;
            if (result.fpart_64b > 10) {
                result.fpart_64b = 10;
                result.number.erase(result.number.begin() + result.ipart_64b + 10, result.number.end());
            }
        }
        return result;
    }
    // n次方根
    inline Unbounded nthRoot(Unbounded opt, uint64_t n) {
        if (n == 0) {
            throw std::runtime_error("0th root undefined");
        }
        if (n % 2 == 0 && opt.sign) {
            throw std::runtime_error("Sqrt of negative number");
        }
        if (opt == hqRead("0") || n == 1) {
            return opt;
        }
        int64_t exp = opt.getExponent();
        Unbounded result = exp10(exp/n);
        result.sign = opt.sign;

        Unbounded two = hqRead("2");
        Unbounded num_n = hqRead(std::to_string(n));
        Unbounded num_n_1 = hqRead(std::to_string(n-1));
        for (auto i = 1; i <= 100; i++) {
            // x = ((n-1)*x + a / x^{n-1}) / n
            result = (num_n_1*result + opt/quickPow(result,n-1)) / num_n;
            if (result.fpart_64b > 10) {
                result.fpart_64b = 10;
                result.number.erase(result.number.begin() + result.ipart_64b + 10, result.number.end());
            }
        }
        return result;
    }
    // n次方根整数部分 大概可以应对 10^50 量级
    inline Unbounded integerRoot(const Unbounded& n, int m) {
        if (n == hqRead("0") || m == 1) return n;

        Unbounded left = hqRead("0");
        Unbounded right = n;
        Unbounded ans = left;
        Unbounded one = hqRead("1");

        while (left < right) {
            Unbounded mid = round2((left + right) / hqRead("2"));
            Unbounded pow_mid = scal::quickPow(mid, m);
            //std::cerr << mid << std::endl;
            if (pow_mid == n) {
                return mid;
            }
            if (pow_mid < n) {
                ans = mid;
                left = mid + one;
            } else {
                right = mid - one;
            }
        }
        if (scal::quickPow(ans + one, m) <= n) {
            ans = ans + one;
        }
        return ans;
    }

    /* --- 三角 --- */
    inline Unbounded cordic(Unbounded /*target*/, uint64_t depth) {
        // 反正切二分表
        std::vector<Unbounded> deg2;
        Unbounded tan2 = hqRead("0.5");
        for (uint64_t i = 1; i <= depth; i++) {
            Unbounded temp;
            for (uint64_t j = 1; j < depth; j++) {
                temp = temp + scal::quickPow(tan2, j*2-1)/hqRead(std::to_string(j*2-1)) - scal::quickPow(tan2, j*2+1)/hqRead(std::to_string(j*2+1));
            }
            deg2.push_back(temp);
            tan2 = tan2 / hqRead("2");
        }
        throw std::logic_error("Scal: CORDIC is not implemented");
    }
    // 正弦
    inline Unbounded sine(uint64_t) { throw std::logic_error("Scal: sine is not implemented"); }
    // 余弦
    inline Unbounded cosine(uint64_t) { throw std::logic_error("Scal: cosine is not implemented"); }
    // 正切
    inline Unbounded tangent(uint64_t) { throw std::logic_error("Scal: tangent is not implemented"); }
    // 余切
    inline Unbounded cotangent(uint64_t) { throw std::logic_error("Scal: cotangent is not implemented"); }
    // 正割
    inline Unbounded secant(uint64_t) { throw std::logic_error("Scal: secant is not implemented"); }
    // 余割
    inline Unbounded cosecant(uint64_t) { throw std::logic_error("Scal: cosecant is not implemented"); }

    /* --- 概率 --- */
    inline Unbounded factorial(const Unbounded& opt) {
        if (opt.fpart_64b != 0) throw std::runtime_error("Factorial of decimal");
        if (opt < hqRead("0")) throw std::runtime_error("Factorial of negative");
        Unbounded one = hqRead("1");    // 迫真 one
        Unbounded result = one;
        for (Unbounded i = one; i <= opt; i = i + one) {
            result = result * i;
        }
        return result;
    }
    // 组合
    inline Unbounded nCr(Unbounded n, Unbounded r) {
        if (r < hqRead("0") || n < r) return hqRead("0");
        if (r == hqRead("0") || r == n) return hqRead("1");

        Unbounded r_copy = n - r < r ? n - r : r;
        Unbounded one = hqRead("1");    // 迫真 one
        Unbounded numerator = one;
        Unbounded denominator = one;
        for (Unbounded i = one; i <= r_copy; i = i + one) {
            numerator = numerator * (n - r_copy + i);
            denominator = denominator * i;
        }
        return round(numerator / denominator,0);
    }
    // 排列
    inline Unbounded nPr(Unbounded n, Unbounded r) {
        if (r < hqRead("0") || n < r) return hqRead("0");
        if (r == hqRead("0")) return hqRead("1");

        Unbounded one = hqRead("1");    // 迫真 one
        Unbounded result = one;
        for (Unbounded i = hqRead("0"); i < r; i = i + one) {
            result = result * (n - i);
        }
        return result;
    }
    // 随机数字
    inline Unbounded randomNumber(Unbounded, Unbounded) {
        throw std::logic_error("Scal: randomNumber is not implemented");
    }
    // 随机字母
    inline Unbounded randomLetter(Unbounded, Unbounded) {
        throw std::logic_error("Scal: randomLetter is not implemented");
    }
}

#endif // SCALRR_BACKEND_HPP
