//---------------------------------------------------------------------------
//#include <exception>
#include <cassert> // 'assert'
#include <iostream> // 'std::cerr'
#include <fstream> // 'std::ifstream'
//#include <regex> // 'std::regex_iterator'
#include "unt_PoorMansUnicode.h" // 'CheckBOM'
//---------------------------------------------------------------------------
#include "unt_Dictionary.h" // 'cls_Dictionary'




//---------------------------------------------------------------------------
cls_Dictionary::cls_Dictionary()
{

}

//---------------------------------------------------------------------------
//cls_Dictionary::~cls_Dictionary()
//{
//
//}

//---------------------------------------------------------------------------
// Invert the dictionary (can exclude numbers)
void cls_Dictionary::Invert(const bool nonum)
{
    std::cout << std::endl << "{Inverting dictionary}" << std::endl;
    if(nonum) std::cerr << "  Excluding numbers" << std::endl;
    inherited inv_map;
    for(auto i=begin(); i!=end(); ++i)
       {
        if( nonum )
           {// Exclude numbers
            try{ ToNum(i->second); continue; } // Skip the number
            catch(...) {} // Ok, not a number
           }
        // Handle aliases, the correct one must be manually chosen
        auto has = inv_map.find(i->second);
        if( has != inv_map.end() )
             {// Already existing, add the alias
              //std::cerr << "  Adding alias \'" << i->first << "\' for " << i->second << std::endl;
              // Add a recognizable placeholder to ease the manual choose
              if(has->second.find("<CHOOSE:")==std::string::npos) has->second = "<CHOOSE:" + has->second + ">";
              has->second[has->second.length()-1] = '|'; // *(has->second.rbegin()) = '|';
              has->second += i->first + ">";
             }
        else {
              auto ins = inv_map.insert( value_type( i->second, i->first ) );
              if( !ins.second )
                 {
                  std::cerr << "  Cannot insert \'" << i->second << "\' !!" << std::endl;
                 }
             }
       }
   std::cout << "  Now got " << inv_map.size() << " voices from previous " << size() << std::endl;
   // Finally, assign the inverted dictionary
   inherited::operator=( inv_map );
} // 'Invert'


//---------------------------------------------------------------------------
// Debug utility
void cls_Dictionary::Peek()
{
    int max = 10;
    for(auto i=begin(); i!=end(); ++i)
       {
        std::cout << "  " << i->first << " " << i->second << std::endl;
        if(--max<0) break;
       }
} // 'Peek'





/////////////////////////////////////////////////////////////////////////////
// A parser for defines that works with different char sizes
template<class TChar> class cls_DictParser //////////////////////////////////
{
 public:
    cls_DictParser<TChar>()
       {// Default constructor
       }
    //~cls_DictParser();

    int Parse(cls_Dictionary& dict, std::istream& fin, TChar cur_c =-1)
       {// Parse file
        enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEFINE, ST_DEF, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
        int issues=0; // Number of issues of the parsing
        int n_def=0; // Number of encountered defines
        int l=1; // Current line number
        std::string def,exp;
        TChar c = cur_c;
        if(c==-1) fin >> c;
        while( fin )
           {
            // Unexpected characters
            //if(c=='\f') { ++issues; std::cerr << pth << "  Unexpected character formfeed in line " << l << std::endl; }
            //else if(c=='\v') { ++issues; std::cerr << pth << "  Unexpected vertical tab in line " << l << std::endl; }
            // According to current status
            switch( status )
               {
                case ST_SKIPLINE : // Skip current line
                    if( c=='\n' )
                       {
                        ++l;
                        status = ST_NEWLINE;
                       }
                    fin >> c; // Next
                    break;

                case ST_NEWLINE : // See what we have
                    while(c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f') fin >> c; // Skip initial spaces
                    // Now see what we have
                    if( c=='#' ) status = ST_DEFINE; // could be a '#define'
                    else if( c=='D' ) status = ST_DEF; // could be a 'DEF'
                    else if( c=='/' && fin.peek()=='/' ) status = ST_SKIPLINE; // TODO: A comment
                    else if( c==';' ) status = ST_SKIPLINE; // A Fagor comment
                    else if( c=='\n') ++l; // Detect possible line break (empty line)
                    else {// Garbage
                          ++issues;
                          std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_NEWLINE)" << std::endl;
                          status = ST_SKIPLINE; // Garbage, skip the line
                         }
                    fin >> c; // Next
                    break;

                case ST_DEFINE : // Detect '#define' directive
                    status = ST_SKIPLINE; // Default: garbage, skip the line
                    if(          c=='d')
                    if(fin>>c && c=='e')
                    if(fin>>c && c=='f')
                    if(fin>>c && c=='i')
                    if(fin>>c && c=='n')
                    if(fin>>c && c=='e')
                       {// Well now should have a space
                        fin >> c;
                        if(c==' ' || c=='\t')
                           {// Got a define
                            status = ST_MACRO;
                            fin >> c; // Next
                           }
                       }
                    // Notify garbage
                    if(status==ST_SKIPLINE)
                       {// Garbage
                        ++issues;
                        std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_DEFINE)" << std::endl;
                       }
                    break;

                case ST_DEF : // Detect 'DEF' directive
                    status = ST_SKIPLINE; // Default: garbage, skip the line
                    if(          c=='E')
                    if(fin>>c && c=='F')
                       {// Well now should have a space
                        fin >> c;
                        if(c==' ' || c=='\t')
                           {// Got a define
                            status = ST_MACRO;
                            fin >> c; // Next
                           }
                       }
                    // Notify garbage
                    if(status==ST_SKIPLINE)
                       {// Garbage
                        ++issues;
                        std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_DEF)" << std::endl;
                       }
                    break;

                case ST_MACRO : // Collect the define macro string
                    while(c==' ' || c=='\t') fin >> c; // Skip spaces
                    // Collect the macro string
                    def = "";
                    // The token ends with control chars or eof
                    while( c!=EOF && c>' ' )
                       {
                        def += c;
                        fin >> c; // Next
                       }
                    if( def.empty() )
                         {// No macro defined!
                          ++issues;
                          std::cerr << "  No macro defined in line " << l << std::endl;
                          status = ST_ENDLINE;
                         }
                    else status = ST_EXP;
                    break;

                case ST_EXP : // Collect the macro expansion string
                    while(c==' ' || c=='\t') fin >> c; // Skip spaces
                    // Collect the expansion string
                    exp = "";
                    // The token ends with control chars or eof
                    while( c!=EOF && c>' ' )
                       {
                        exp += c;
                        fin >> c; // Next
                       }
                    if( exp.empty() )
                         {// No expansion defined!
                          ++issues;
                          std::cerr << "  No expansion defined in line " << l << std::endl;
                         }
                    else {
                          auto ins = dict.insert( cls_Dictionary::value_type( def, exp ) );
                          if( !ins.second )
                             {
                              ++issues;
                              std::cerr << "  \'" << ins.first->first << "\' already existed in line " << l << " (was \'" << ins.first->second << "\') " << std::endl;
                             }
                          else ++n_def;
                         }
                    status = ST_ENDLINE;
                    break;

                case ST_ENDLINE : // Check the remaining after a macro definition
                    while(c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f') fin >> c; // Skip final spaces
                    // Now see what we have
                    if(c=='\n') { ++l; status=ST_NEWLINE; } // Detect expected line break
                    else if( c=='/' && fin.peek()=='/' ) status = ST_SKIPLINE; // A comment
                    else if( c==';' ) status = ST_SKIPLINE; // A Fagor comment
                    else {// Garbage
                          ++issues;
                          std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_ENDLINE)" << std::endl;
                          status = ST_SKIPLINE; // Garbage, skip the line
                         }
                    fin >> c; // Next
                    break;

               } // 'switch(status)'
           } // 'while(!EOF)'
        // Finally
        std::cout << "  Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << " char size:" << sizeof(TChar) << std::endl;
        return issues;
       } // 'Parse'

}; // 'cls_DictParser'

/////////////////////////////////////////////////////////////////////////////
// A parser specialization for plain 'char'
template<> class cls_DictParser<char> ///////////////////////////////////////
{
 public:
    cls_DictParser()
       {// Default constructor
       }
    //~cls_DictParser();

    int Parse(cls_Dictionary& dict, std::istream& fin, char& c)
       {// Parse file
        enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEFINE, ST_DEF, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
        int issues=0; // Number of issues of the parsing
        int n_def=0; // Number of encountered defines
        int l=1; // Current line number
        std::string def,exp;
        while( c != EOF )
           {
            // Unexpected characters
            //if(c=='\f') { ++issues; std::cerr << pth << "  Unexpected character formfeed in line " << l << std::endl; }
            //else if(c=='\v') { ++issues; std::cerr << pth << "  Unexpected vertical tab in line " << l << std::endl; }
            // According to current status
            switch( status )
               {
                case ST_SKIPLINE : // Skip current line
                    if( c=='\n' )
                       {
                        ++l;
                        status = ST_NEWLINE;
                       }
                    c = fin.get();
                    break;

                case ST_NEWLINE : // See what we have
                    while(c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f') c=fin.get(); // Skip initial spaces
                    // Now see what we have
                    if( c=='#' ) status = ST_DEFINE; // could be a '#define'
                    else if( c=='D' ) status = ST_DEF; // could be a 'DEF'
                    else if( c=='/' && fin.peek()=='/' ) status = ST_SKIPLINE; // A comment
                    else if( c==';' ) status = ST_SKIPLINE; // A Fagor comment
                    else if(c=='\n') ++l; // Detect possible line break (empty line)
                    //else if(c=='\0') c=fin.get(); // Skip null chars (UTF-16/32 encodings)
                    else {// Garbage
                          ++issues;
                          std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_NEWLINE)" << std::endl;
                          status = ST_SKIPLINE; // Garbage, skip the line
                         }
                    c = fin.get(); // Next
                    break;

                case ST_DEFINE : // Detect '#define' directive
                    status = ST_SKIPLINE; // Default: garbage, skip the line
                    if(            c=='d')
                    if((c=fin.get())=='e')
                    if((c=fin.get())=='f')
                    if((c=fin.get())=='i')
                    if((c=fin.get())=='n')
                    if((c=fin.get())=='e')
                       {// Well now should have a space
                        c = fin.get();
                        if(c==' ' || c=='\t')
                           {// Got a define
                            status = ST_MACRO;
                            c = fin.get(); // Next
                           }
                       }
                    // Notify garbage
                    if(status==ST_SKIPLINE)
                       {// Garbage
                        ++issues;
                        std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_DEFINE)" << std::endl;
                       }
                    break;

                case ST_DEF : // Detect 'DEF' directive
                    status = ST_SKIPLINE; // Default: garbage, skip the line
                    if(            c=='E')
                    if((c=fin.get())=='F')
                       {// Well now should have a space
                        c = fin.get();
                        if(c==' ' || c=='\t')
                           {// Got a define
                            status = ST_MACRO;
                            c = fin.get(); // Next
                           }
                       }
                    // Notify garbage
                    if(status==ST_SKIPLINE)
                       {// Garbage
                        ++issues;
                        std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_DEF)" << std::endl;
                       }
                    break;

                case ST_MACRO : // Collect the define macro string
                    while(c==' ' || c=='\t') c=fin.get(); // Skip spaces
                    // Collect the macro string
                    def = "";
                    // The token ends with control chars or eof
                    while( c!=EOF && c>' ' )
                       {
                        def += c;
                        c = fin.get(); // Next
                       }
                    if( def.empty() )
                         {// No macro defined!
                          ++issues;
                          std::cerr << "  No macro defined in line " << l << std::endl;
                          status = ST_ENDLINE;
                         }
                    else status = ST_EXP;
                    break;

                case ST_EXP : // Collect the macro expansion string
                    while(c==' ' || c=='\t') c=fin.get(); // Skip spaces
                    // Collect the expansion string
                    exp = "";
                    // The token ends with control chars or eof
                    while( c!=EOF && c>' ' )
                       {
                        exp += c;
                        c = fin.get(); // Next
                       }
                    if( exp.empty() )
                         {// No expansion defined!
                          ++issues;
                          std::cerr << "  No expansion defined in line " << l << std::endl;
                         }
                    else {
                          auto ins = dict.insert( cls_Dictionary::value_type( def, exp ) );
                          if( !ins.second )
                             {
                              ++issues;
                              std::cerr << "  \'" << ins.first->first << "\' already existed in line " << l << " (was \'" << ins.first->second << "\') " << std::endl;
                             }
                          else ++n_def;
                         }
                    status = ST_ENDLINE;
                    break;

                case ST_ENDLINE : // Check the remaining after a macro definition
                    while(c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f') c=fin.get(); // Skip final spaces
                    // Now see what we have
                    if(c=='\n') { ++l; status=ST_NEWLINE; } // Detect expected line break
                    else if( c=='/' && fin.peek()=='/' ) status = ST_SKIPLINE; // A comment
                    else if( c==';' ) status = ST_SKIPLINE; // A Fagor comment
                    else {// Garbage
                          ++issues;
                          std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_ENDLINE)" << std::endl;
                          //char s[256]; fin.getline(s,256); std::cerr << s << std::endl;
                          status = ST_SKIPLINE; // Garbage, skip the line
                         }
                    c = fin.get(); // Next
                    break;

               } // 'switch(status)'
           } // 'while(!EOF)'
        // Finally
        std::cout << "  Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << std::endl;
        return issues;
       } // 'Parse'

}; // 'cls_DictParser<char>'



//---------------------------------------------------------------------------
// A simple fast parser for:
// #define XXX YYY // comment
// DEF XXX YYY ; comment
int cls_Dictionary::LoadFile( const std::string& pth )
{
    // (0) See syntax (extension)
    //std::string ext( pth.find_last_of(".")!=std::string::npos ? pth.substr(pth.find_last_of(".")+1) : "" );
    //if( ext=="def" )

    // (1) Open file for read
    std::cout << std::endl << '[' << pth << ']' << std::endl;
    std::ifstream fin( pth, std::ios::binary );
    if( !fin )
       {
        std::cerr << "  Cannot read the file!" << std::endl;
        return 1;
       }

    // (2) Parse the file
    // Poor man's dealing with UTF-16 encoding check possible BOM
    char c;
    EN_ENCODING enc = CheckBOM( c, fin );
    // Get the rest
    if( enc == ANSI )
         {
          cls_DictParser<char> parser;
          return parser.Parse(*this, fin, c);
         }
    if( enc == UTF16 )
         {
          cls_DictParser<char16_t> parser;
          return parser.Parse(*this, fin, c);
         }
    else {
          std::cerr << "  Cannot handle this encoding!" << std::endl;
          return 1;
         }
} // 'LoadFile'


/*
//---------------------------------------------------------------------------
// Using regular expression
int cls_Dictionary::LoadFile( const std::string& pth )
{
    // (0) See syntax (extension)
    //std::string ext( pth.find_last_of(".")!=std::string::npos ? pth.substr(pth.find_last_of(".")+1) : "" );
    //if( ext=="def" )
    std::regex regdef( R"(^\s*(?:#define|DEF)\s+(\S+)\s+(\S+))" );

    // (1) Open file for read
    std::ifstream fin( pth );
    if( !fin )
       {
        std::cerr << "!! Cannot open " << pth << std::endl;
        return 1;
       }

    // (2) Scan lines
    int issues = 0;
    std::string line;
    while( std::getline(fin, line) )
       {
        //std::stringstream lineStream(line);
        //std::string token;
        //while(lineStream >> token) std::cout << "Token: " << token << std::endl;

        std::regex_iterator<std::string::iterator> rit ( line.begin(), line.end(), regdef ), rend;
        //while( rit!=rend ) { std::cout << rit->str() << std::endl; ++rit; }
        if( rit!=rend )
           {
            //std::cout << '\n' for(size_type i=0; i<rit->size(); ++i) std::cout << rit->str(i) << '\n';
            assert( rit->size()==3 );
            auto ret = insert( value_type( rit->str(1), rit->str(2) ) );
            if( !ret.second )
               {
                std::cerr << "\'" << ret.first->first << "\' already existed (was \'" << ret.first->second << "\') " << std::endl;
                ++issues;
               }
           }
       }
    fin.close();

    // (3) Finally
    return issues;
} // 'LoadFile'
*/


//---------------------------------------------------------------------------
// Strictly convert a (dec) floating point or an (oct/dec/hex) int literal
double cls_Dictionary::ToNum( const std::string& s, const bool strict )
{
    if(strict) throw std::runtime_error("bad");
    return 0;
/*
    // . Internal variables
    double m=0; int sm=1; // mantissa and its sign
    int e=0, se=1; // exponent and its sign

    bool i_found = false; // integer part found
    bool f_found = false; // fractional part found
    bool e_found = false; // exponential part found
    bool expchar_found= false; // exponential character found

    // . Get integer part sign
    if ( readchar == '-' ) {s = -1; NextChar();}
    else if ( readchar == '+' ) NextChar(); // s = 1;

    // . Get integer part
    if ( readchar >= '0' && readchar <= '9' )
       {
        i_found = true;
        do {
            m = (10.0*m) + (readchar - '0');
            while ( NextChar() == chThousandSep ); // Skip thousand separator char
           }
        while ( readchar >= '0' && readchar <= '9' );
       }

    // . Get decimal part
    if ( readchar == chDecimalSep )
       {
        NextChar();
        double k = 0.1; // shift of decimal part
        if ( readchar >= '0' && readchar <= '9' )
           {
            f_found = true;
            do {
                m += k*double(readchar - '0');
                k *= 0.1;
                NextChar();
               }
            while ( readchar >= '0' && readchar <= '9' );
           }
       }

    // . Get exponential part
    if ( readchar == 'E' || readchar == 'e' )
       {
        expchar_found = true;
        NextChar();
        // sign
        if ( readchar == '-' ) {se = -1; NextChar();}
        else if ( readchar == '+' ) NextChar(); // se = 1;
        // exponent
        if ( readchar >= '0' && readchar <= '9' )
           {
            e_found = true;
            do {
                e = (10.0*e) + (readchar - '0');
                NextChar();
               }
            while ( readchar >= '0' && readchar <= '9' );
           }
       }

    // . Number finished: encountered a delimiter or no more chars
    // Calculate value
    double value;

    if ( i_found || f_found )
         {
          if ( e_found ) value = s*m*std::pow10(se*e); // All part given
          else if ( expchar_found && strict ) throw EParsingError("Invalid number", cls_ParseRegion(i_start,Position(),Line(),Col()));
          else value = s*m; // no exp part
         }
    else {
          if ( e_found )
               {
                value = s*std::pow10(se*e); // Only exponential
               }
          else {// No part at all
                if( strict ) throw EParsingError("Invalid number", cls_ParseRegion(i_start,Position(),Line(),Col()));
                if(expchar_found) value = 1; // things like 'E,+E,-e,e+,E-,...'
                else value = mat::NaN; // things like '+,-,,...'
               }
         }

    // . Finally
    return value;


    // ReadInt ============================================
    // . Variables
    int value = 0;
    int sign = 1;
    int base = 10;
    //static bool strict = true; // Settings
    //int i_start = Position();

    // . Get sign
    if ( readchar == '-' ) {sign = -1; NextChar();}
    else if ( readchar == '+' ) NextChar(); // sign = 1;

    // . Get prefix
    if ( readchar == '0' ) { base = 8; NextChar(); }
    if ( readchar == 'x' || readchar == 'X' ) { base = 16; NextChar(); }

    // . Get integer part value
    TChar offset;
    switch ( base )
       {
        case 8 :
            offset = '0';
            while ( NotEnded() )
               {
                if ( readchar >= '0' && readchar <= '7' ) value = (base*value) + (readchar - offset);
                else break;
                while ( NextChar() == chThousandSep ); // Skip thousand separator char
               }
            break;

        case 16 :
            while ( NotEnded() )
               {
                if ( readchar >= '0' && readchar <= '9' ) offset = '0';
                else if ( readchar >= 'A' && readchar <= 'F' ) offset = 'A' - 10;
                else if ( readchar >= 'a' && readchar <= 'f' ) offset = 'a' - 10;
                else break;
                value = (base*value) + (readchar - offset);
                while ( NextChar() == chThousandSep ); // Skip thousand separator char
               }
            break;

        default : // 10
            offset = '0';
            while ( NotEnded() )
               {
                if ( readchar >= '0' && readchar <= '9' ) value = (base*value) + (readchar - offset);
                else break;
                while ( NextChar() == chThousandSep ); // Skip thousand separator char
               }
       }

    // . Number finished: encountered a delimiter or no more chars
    //if ( !i_found && strict ) throw EParsingError("Invalid number", cls_ParseRegion(i_start,Position(),Line(),Col()));

    // . Finally
    return sign * value;
*/
} // 'ToNum'
