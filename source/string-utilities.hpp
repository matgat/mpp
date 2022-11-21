#ifndef GUARD_string_utilities_hpp
#define GUARD_string_utilities_hpp
//  ---------------------------------------------
//  String utilities
//  ---------------------------------------------
#include <cctype> // std::isdigit, ...
#include <string>
#include <string_view>
//#include <optional>
#include <charconv> // std::from_chars
#include "logging.hpp" // dlg::error


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
// Change string to lowercase
inline std::string tolower(std::string s)
{
    for(char& c : s) c = static_cast<char>(std::tolower(c));
    // With c++20 ranges:
    //s |= action::transform([](unsigned char c){ return std::tolower(c); });
    return s;
}


//---------------------------------------------------------------------------
// Convert a character to string, escaping it if necessary
std::string escape(const char c) noexcept
   {
    std::string s(1,c);
    switch( c )
       {
        case '\n': s = "\\n"; break;
        case '\r': s = "\\r"; break;
        case '\t': s = "\\t"; break;
        //case '\f': s = "\\f"; break;
        //case '\v': s = "\\v"; break;
        //case '\0': s = "\\0"; break;
       }
    return s;
   }


//---------------------------------------------------------------------------
// Show string special characters
std::string& escape(std::string& s) noexcept
   {
    std::string::size_type i = 0;
    while( i<s.size() )
       {
        switch( s[i] )
           {
            case '\n': s.replace(i, 2, "\\n"); i+=2; break;
            case '\r': s.replace(i, 2, "\\r"); i+=2; break;
            case '\t': s.replace(i, 2, "\\t"); i+=2; break;
            //case '\f': s.replace(i, 2, "\\f"); i+=2; break;
            //case '\v': s.replace(i, 2, "\\v"); i+=2; break;
            //case '\0': s.replace(i, 2, "\\0"); i+=2; break;
            default : ++i;
           }
       }
    return s;
   }


//---------------------------------------------------------------------------
// Show string special characters
std::string escape(const std::string_view sv) noexcept
   {
    std::string s(sv);
    return escape(s);
   }


//---------------------------------------------------------------------------
// replace all occurrences in a string
void replace_all(std::string& s, const std::string& from, const std::string& to)
{
    //std::string::size_type i = 0;
    //while( (i = s.find(from, i)) != std::string::npos )
    //   {
    //    s.replace(i, from.length(), to);
    //    i += to.length();
    //   }

    std::string sout;
    sout.reserve(s.length());
    std::string::size_type i, i_start=0;
    while( (i = s.find(from, i_start)) != std::string::npos )
       {
        sout.append(s, i_start, i-i_start);
        sout.append(to);
        i_start = i + from.length();
       }
    sout.append(s, i_start); //, std::string::npos);
    s.swap(sout);
}


//---------------------------------------------------------------------------
// Convert string_view to int
int to_int(const std::string_view s)
{
    int result;
    auto [p, ec] = std::from_chars(s.data(), s.data()+s.size(), result);
    //if(ec == std::errc::runtime_error || ec == std::errc::result_out_of_range)
    if( ec != std::errc() || p!=(s.data()+s.size()) ) throw dlg::error("\"{}\" is not a valid integer",s);
    return result;
}


//---------------------------------------------------------------------------
// Convert string_view to double
//double to_double(const std::string_view s)
//{
//    double result;
//    auto [p, ec] = std::from_chars(s.data(), s.data()+s.size(), result);
//    //if(ec == std::errc::runtime_error || ec == std::errc::result_out_of_range)
//    if( ec != std::errc() || p!=(s.data()+s.size()) ) throw dlg::error("\"{}\" is not a valid double",s);
//    return result;
//}


//---------------------------------------------------------------------------
// Convert string_view to int
//std::optional<int> to_int(const std::string_view s)
//   {
//    int result;
//    const std::from_chars_result res = std::from_chars(s.data(), s.data() + s.size(), result);
//    if(res.ec == std::errc()) return result;
//    return std::nullopt;
//   }


//---------------------------------------------------------------------------
// Generate an hash for a string
std::size_t hash(const std::string_view s)
{
    std::size_t val = 0;
    //for(const char c : s) crc += static_cast<std::size_t>(c);
    for(std::size_t i=0; i<s.length(); ++i) val += (s.length() - i) * static_cast<std::size_t>(s[i]);
    return val;
}




/////////////////////////////////////////////////////////////////////////////
// Natural sorting compare functor
// std::map<std::string,std::string,NatSortLess> map;
// std::sort(vector, NatSortLess cmp);
class NatSortLess ///////////////////////////////////////////////////////////
{
 public:
    bool operator()(const char* const a, const char* const b) const { return natcmp(a, b) < 0; }
    bool operator()(const std::string& a, const std::string& b) const { return natcmp(a.c_str(), b.c_str()) < 0; }
    //bool operator()(const std::string_view a, const std::string_view b) const { return natcmp(a, b) < 0; }

 private:
    //-----------------------------------------------------------------------
    // Natural compare function
    static int natcmp(const char* const a, const char* const b, const bool no_case =true)
       {
        int ia=0, ib=0;
        while(true)
           {
            // Skip leading spaces
            while(std::isspace(a[ia])) ++ia;
            while(std::isspace(b[ib])) ++ib;

            char ca = a[ia];
            char cb = b[ib];

            if( std::isdigit(ca) && std::isdigit(cb) )
               {// Compare numbers
                int result = (ca=='0' || cb=='0') ? cmp_left(a+ia, b+ib) : cmp_right(a+ia, b+ib);
                if(result!=0) return result;
               }

            // If both terminated, have the same order
            if(!ca && !cb) return 0; // could call 'std::strcmp' for further check

            if(no_case)
               {
                ca = static_cast<char>(std::toupper(ca));
                cb = static_cast<char>(std::toupper(cb));
               }

            // Check sorting
            if(ca<cb) return -1;
            else if(ca>cb) return +1;

            // Next chars
            if(ca) ++ia;
            if(cb) ++ib;
           }
       }

    //-----------------------------------------------------------------------
    // The longest run of digits wins.
    // That aside, the greatest value wins, but we can't know that it will
    // until we've scanned both numbers to know that they have the
    // same magnitude, so we remember it in BIAS.
    static inline int cmp_right(const char* a, const char* b)
       {
        int bias = 0;
        while(true)
           {
            if(std::isdigit(*a)) { if(!std::isdigit(*b)) return +1; }
            else return std::isdigit(*b) ? -1 : bias;

            // If here the strings are both digits, so not yet terminated
            if(*a<*b) { if(!bias) bias = -1; }
            else if(*a>*b) { if(!bias) bias = +1; }
            //else if(!*a && !*b) return bias; // this should never be true
            ++a, ++b;
           }
       }

    //-----------------------------------------------------------------------
    // Compare two left-aligned numbers: the first to have a different value wins
    static inline int cmp_left(const char* a, const char* b)
       {
        while(true)
           {
            if(std::isdigit(*a)) { if(!std::isdigit(*b)) return +1; }
            else return std::isdigit(*b) ? -1 : 0;

            // If here the strings are both digits, so not yet terminated
            if(*a<*b) return -1;
            else if(*a>*b) return +1;
            ++a, ++b;
           }
       }
};





//template<class Int> constexpr typename std::enable_if<std::is_unsigned<Int>::value, Int>::type make_mask(const unsigned char pattern)
//   {
//    return ((std::numeric_limits<Int>::max() / std::numeric_limits<unsigned char>::max()) * pattern);
//   }

//---------------------------------------------------------------------------
//consteval std::string indent(const std::size_t n)
//{
//    return std::string(n, "\t");
//}



/////////////////////////////////////////////////////////////////////////////
// A compile time string
//class static_string
//{
// public:
//    template<std::size_t N> consteval static_string(const char(&a)[N]) : ptr(a), len(N-1) {}
//    //consteval char operator[](const std::size_t n)
//    //   {
//    //    return n < len ? ptr[n] :
//    //    throw std::out_of_range("");
//    //   }
//    //consteval std::size_t size() { return len; }
//    consteval std::string_view view() const noexcept { return std::string_view(ptr,len); }
//
// private:
//    constinit char* const ptr;
//    constinit std::size_t len;
//};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
