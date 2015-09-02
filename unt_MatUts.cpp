//#include <exception>
#include <stdexcept> // 'std::runtime_error'
//#include <cassert> // 'assert'
#include <cmath> // 'std::pow'
//#include <iostream> // 'std::cerr'
//#include <fstream> // 'std::ifstream'
//---------------------------------------------------------------------------
#include "unt_MatUts.h" // 'nms_Mat'



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
    double val = 0.0;

    // (2.1) Get integer part
    if( s[i]>='0' && s[i]<='9' )
       {
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
        //e_found = true;
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
    //if(strict) throw std::runtime_error("Invalid number: " + s);
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
