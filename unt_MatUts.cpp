//#include <exception>
#include <stdexcept> // 'std::runtime_error'
//#include <cassert> // 'assert'
#include <cmath> // 'std::pow'
//#include <iostream> // 'std::cerr'
//#include <fstream> // 'std::ifstream'
//---------------------------------------------------------------------------
#include "unt_MatUts.h" // 'nms_Mat'


//-----------------------------------------------------------------------
// Decompose path
void nms_Mat::split_path(const std::string& pth, std::string& dir, std::string& nam, std::string& ext)
{
    // Get parent folder and fullname
    std::string::size_type i = pth.find_last_of("\\/");
    if( i != std::string::npos )
         {
          dir = pth.substr(0, ++i);
          nam = pth.substr(i);
         }
    else {
          dir = "";
          nam = pth;
         }
    // Get basename and extension (dot included)
    i = nam.rfind('.');
    if( i != std::string::npos )
         {
          ext = nam.substr(i);
          nam = nam.substr(0,i);
         }
    else {
          ext = "";
         }
} // 'split_path'



/*
//---------------------------------------------------------------------------
// Get/set the modification date of a file
#include <boost/filesystem/operations.hpp>
#include <ctime>
void nms_Mat::change_mdate( const std::string& pth )
{
    boost::filesystem::path opth( pth ) ;
    if( boost::filesystem::exists( opth ) )
       {
        std::time_t mt = boost::filesystem::last_write_time( opth ) ;
        //std::cout << pth << " was modified on " << std::ctime(&mt) << '\n';
        std::time_t now = std::time(0);
        boost::filesystem::last_write_time( opth, now );
        //mt = boost::filesystem::last_write_time( opth );
        //std::cout << "Now the modification time is " << std::ctime(&mt) << '\n';
       }
    //else std::cerr << "Could not find file " << pth << '\n';
    else throw std::runtime_error("Could not find file: " + pth);

    //  WinApi solutions:
    //  FILE_BASIC_INFO and SetFileInformationByHandle
    //  FILETIME and SetFileTime
    //    FILETIME ft;
    //    // SYSTEMTIME st; ::SystemTimeToFileTime(&st, &ft);
    //    HANDLE h = ::CreateFile(sPath, GENERIC_ALL, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    //    if( h!=INVALID_HANDLE_VALUE )
    //       {
    //        BOOL ret = ::SetFileTime(h, &ft, &ft, &ft); // Use NULL in CreationTime,LastAccessTime,LastWriteTime
    //        ::CloseHandle(h);
    //       }
} // 'change_mdate'
*/


//---------------------------------------------------------------------------
// Strictly convert a (dec) floating point or an (oct/dec/hex) int literal
double nms_Mat::ToNum( const std::string& s, const bool strict )
{
    // Cheating: detect if strict integer
    //std::string::size_type p = s.find_first_of(".eE") if( p==std::string::npos )
    const char chDecSep = '.';
    const char chThnSep = '\'';

    size_t i=0, len=s.length(); // current character

    // (0) Skip initial control chars
    //while( i<len ) if(s[i]<=' ') ++i; else break;

    // (1) See first part
    if(i>=len) if(strict) throw std::runtime_error("Invalid number (empty string)");
    // Detect sign if present
    double sgn = 1; // sign
    if( s[i]=='-' )
       {
        sgn = -1;
        ++i;
        if(i>=len) if(strict) throw std::runtime_error("Invalid number: " + s);
       }
    else if( s[i]=='+' )
       {
        ++i;
        if(i>=len) if(strict) throw std::runtime_error("Invalid number: " + s);
       }
    // Detect base if present (hexadecimal or octal integer)
    if( s[i]=='0' )
       {
        ++i;
        if(i>=len) return 0.0;
        // See char after '0'
        if(s[i]=='x' || s[i]=='X')
           {// Expecting a hexadecimal integer
            int val = 0;
            // Get the rest of the hex number
            while( ++i<len )
               {
                if ( s[i]>='0' && s[i]<='9' ) val = (16*val) + (s[i]-'0');
                else if ( s[i]>='A' && s[i]<='F' ) val = (16*val) + (s[i]-'A'+10);
                else if ( s[i]>='a' && s[i]<='f' ) val = (16*val) + (s[i]-'a'+10);
                //else if(s[i]==chThnSep) continue; // Skip thousand separator char
                else throw std::runtime_error("Invalid hexadecimal integer: " + s);
               }
            return sgn * val;
           } // '0x'
        else if( s[i]>='0' && s[i]<='7' )
           {// Expecting an octal integer
            int val = s[i] - '0';
            // Get the rest of the octal number
            while( ++i<len )
               {
                if( s[i]>='0' && s[i]<='7' ) val = (8*val) + (s[i]-'0');
                //else if(s[i]==chThnSep) continue; // Skip thousand separator char
                else throw std::runtime_error("Invalid octal integer: " + s);
               }
            return sgn * val;
           } // '0'
       }

    // (2) Floating point number
    int found = 0;
    double val = 0.0;

    // (2.1) Get integer part
    if( s[i]>='0' && s[i]<='9' )
       {
        found |= 0x1; // Found integer part
        val = s[i] - '0';
        while( ++i<len )
           {
            if( s[i]>='0' && s[i]<='9' ) val = 10.0*val + (s[i]-'0');
            else if( s[i]==chDecSep ) break; // Integer part done
            else if( s[i]=='E' || s[i]=='e' ) break; // Integer part done
            else if( s[i]==chThnSep ) continue; // Skip thousand separator char
            else throw std::runtime_error("Invalid number: " + s);
           }
       }

    // (2.2) Get decimal part
    double k = 0.1; // shift of decimal part
    if( s[i]==chDecSep )
       {
        found |= 0x2; // Found decimal sep
        size_t di = i;
        while( ++i<len )
           {
            if( s[i]>='0' && s[i]<='9' ) { val += k*(s[i]-'0'); k*=0.1; }
            else if( s[i]=='E' || s[i]=='e' ) break; // Decimal part done
            else throw std::runtime_error("Invalid number: " + s);
           }
        if((i-di)<2 && strict) throw std::runtime_error("Invalid number: " + s);
       }

    // (2.3) Get exponential part
    int exp=0, exp_sgn=1; // exponent and its sign
    if( s[i]=='E' || s[i]=='e' )
       {
        found |= 0x4; // Found exponential
        ++i;
        if(i>=len) if(strict) throw std::runtime_error("Invalid number: " + s);
        // Detect exponent sign
        if( s[i]=='-' )
           {
            exp_sgn = -1;
            ++i;
            if(i>=len) if(strict) throw std::runtime_error("Invalid number: " + s);
           }
        else if( s[i]=='+' )
           {
            ++i;
            if(i>=len) if(strict) throw std::runtime_error("Invalid number: " + s);
           }
        // Get exponent
        do {
            if( s[i]>='0' && s[i]<='9' ) exp = (10*exp) + (s[i] - '0');
            else throw std::runtime_error("Invalid number: " + s);
           }
        while( ++i<len );
       }

    // (2.4) Check, calculate and return value
    if(!found) throw std::runtime_error("Invalid number: " + s);
    if(exp) return sgn * val * std::pow(10, exp_sgn * exp);
    else return sgn * val;
} // 'ToNum'

/*
    #define TRY(X) try{std::cout << X << " = " << mat::ToNum(X) << "\n";} catch(std::exception& e) { std::cout << e.what() << "\n"; }
    TRY("123E-2")
    TRY("1")
    TRY("1.0")
    TRY("0.0")
    TRY("0.00")
    TRY("1.123")
    TRY("0.123E2")
    TRY("0xFF")
    TRY("073")
    TRY("083")
    TRY("0")
    TRY("0.123")
    TRY("0.")
    TRY(".3")
    TRY(".12")
    TRY(".")
    TRY("1a")
    TRY("0xE3")
    TRY(".E3")
*/
