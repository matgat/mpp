//#include <stdexcept> // 'std::runtime_error'
#include <cassert> // 'assert'
#include <iostream> // 'std::cerr'
#include <fstream> // 'std::ifstream'
//#include <regex> // 'std::regex_iterator'
#include "unt_PoorMansUnicode.h" // 'CheckBOM'
#include "unt_MatUts.h" // 'mat::ToNum'
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
    std::cout << std::endl << "{Inverting dictionary}\n";
    if(nonum) std::cerr << "  Excluding numbers\n";
    inherited inv_map;
    for( auto i=begin(); i!=end(); ++i )
       {
        if( nonum )
           {// Exclude numbers
            try{ mat::ToNum(i->second); continue; } // Skip the number
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
                  std::cerr << "  Cannot insert \'" << i->second << "\' !!\n";
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
// A function object that read a char from byte stream
template<typename T,bool E=true> class fun_ReadChar /////////////////////////
{
 public:
    fun_ReadChar()
       {
        std::cerr << "  Reading stream unit: " << sizeof(T) << " bytes\n";
       }
    inline std::istream& operator()(T& c, std::istream& in)
       {
        in.read(buf,bufsiz);
        if(E){// Little endian
              c = buf[0];
              for(int i=1; i<bufsiz; ++i) c |= buf[i] << (8*i);
             } 
        else {// Big endian
              int imax = bufsiz-1;
              for(int i=0; i<imax; ++i) c |= buf[i] << (8*(imax-i));
              c |= buf[imax];
             }
        return in;
       }
 private:
    static const int bufsiz = sizeof(T);
    mutable char buf[bufsiz];
}; // 'fun_ReadChar'
/////////////////////////////////////////////////////////////////////////////
// Specialization for 32bit chars little endian
template<> class fun_ReadChar<char32_t,true> ////////////////////////////////
{
 public:
    fun_ReadChar()
       {
        std::cerr << "  Reading stream unit: char32_t little endian\n";
       }
    inline std::istream& operator()(char32_t& c, std::istream& in)
       {
        in.read(buf,2);
        c = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24); // little endian
        return in;
       }
 private:
    mutable char buf[4];
}; // 'fun_ReadChar'
/////////////////////////////////////////////////////////////////////////////
// Specialization for 32bit chars big endian
template<> class fun_ReadChar<char32_t,false> ///////////////////////////////
{
 public:
    fun_ReadChar()
       {
        std::cerr << "  Reading stream unit: char32_t big endian\n";
       }
    inline std::istream& operator()(char32_t& c, std::istream& in)
       {
        in.read(buf,2);
        c = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]; // big endian
        return in;
       }
 private:
    mutable char buf[4];
}; // 'fun_ReadChar'
/////////////////////////////////////////////////////////////////////////////
// Specialization for 16bit chars little endian
template<> class fun_ReadChar<char16_t,true> ////////////////////////////////
{
 public:
    fun_ReadChar()
       {
        std::cerr << "  Reading stream unit: char16_t little endian\n";
       }
    inline std::istream& operator()(char16_t& c, std::istream& in)
       {
        in.read(buf,2);
        c = buf[0] | (buf[1] << 8); // little endian
        return in;
       }
 private:
    mutable char buf[2];
}; // 'fun_ReadChar'
/////////////////////////////////////////////////////////////////////////////
// Specialization for 16bit chars big endian
template<> class fun_ReadChar<char16_t,false> ///////////////////////////////
{
 public:
    fun_ReadChar()
       {
        std::cerr << "  Reading stream unit: char16_t big endian\n";
       }
    inline std::istream& operator()(char16_t& c, std::istream& in)
       {
        in.read(buf,2);
        c = (buf[0] << 8) | buf[1]; // big endian
        return in;
       }
 private:
    mutable char buf[2];
}; // 'fun_ReadChar'
/////////////////////////////////////////////////////////////////////////////
// Specialization for 8bit chars
template<> class fun_ReadChar<char> /////////////////////////////////////////
{
 public:
    fun_ReadChar()
       {
        //std::cerr << "  Reading stream unit: char\n";
       }
    inline std::istream& operator()(char& c, std::istream& in)
       {
        return in.get(c);
       }
}; // 'fun_ReadChar'


//---------------------------------------------------------------------------
// A parser for c-like '#define' directives
template<typename T,bool E=true> int Parse_H(cls_Dictionary& dict, std::istream& fin)
{
    fun_ReadChar<T,E> get;
    //std::cout << "  Parsing as: C-like defines\n";
    
    enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEFINE, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
    int issues=0; // Number of issues of the parsing
    int n_def=0; // Number of encountered defines
    int l=1; // Current line number
    std::string def,exp;
    T c;
    get(c,fin);
    while( fin )
       {
        // According to current status
        switch( status )
           {
            case ST_SKIPLINE : // Skip current line
                if( c=='\n' )
                   {
                    ++l;
                    status = ST_NEWLINE;
                   }
                get(c,fin); // Next
                break;

            case ST_NEWLINE : // See what we have
                while(c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f') get(c,fin); // Skip initial spaces
                // Now see what we have
                if( c=='#' ) status = ST_DEFINE; // could be a '#define'
                else if( c=='/' && fin.peek()=='/' ) status = ST_SKIPLINE; // A comment
                else if( c=='\n' ) ++l; // Detect possible line break (empty line)
                else {// Garbage
                      ++issues;
                      std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_NEWLINE)\n";
                      status = ST_SKIPLINE; // Garbage, skip the line
                     }
                get(c,fin); // Next
                break;

            case ST_DEFINE : // Detect '#define' directive
                status = ST_SKIPLINE; // Default: garbage, skip the line
                if(              c=='d')
                if(get(c,fin) && c=='e')
                if(get(c,fin) && c=='f')
                if(get(c,fin) && c=='i')
                if(get(c,fin) && c=='n')
                if(get(c,fin) && c=='e')
                   {// Well now should have a space
                    get(c,fin);
                    if(c==' ' || c=='\t')
                       {// Got a define
                        status = ST_MACRO;
                        get(c,fin); // Next
                       }
                   }
                // Notify garbage
                if(status==ST_SKIPLINE)
                   {// Garbage
                    ++issues;
                    std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_DEFINE)\n";
                   }
                break;

            case ST_MACRO : // Collect the define macro string
                while(c==' ' || c=='\t') get(c,fin); // Skip spaces
                // Collect the macro string
                def = "";
                // The token ends with control chars or EOF
                while( c!=EOF && c>' ' )
                   {
                    // if(c=='/') invalid macro chars
                    def += c;
                    get(c,fin); // Next
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
                while(c==' ' || c=='\t') get(c,fin); // Skip spaces
                // Collect the expansion string
                exp = "";
                // The token ends with control chars or EOF
                // ...or get all, until comment, EOL or EOF
                while( c!=EOF && c>' ' )
                   {
                    exp += c;
                    get(c,fin); // Next
                   }
                if( exp.empty() )
                     {// No expansion defined!
                      ++issues;
                      std::cerr << "  No expansion defined in line " << l << std::endl;
                     }
                else {
                      // Insert in dictionary
                      auto ins = dict.insert( cls_Dictionary::value_type( def, exp ) );
                      if( !ins.second )
                         {
                          ++issues;
                          std::cerr << "  \'" << ins.first->first << "\' already existed in line " << l << " (was \'" << ins.first->second << "\', now \'" << exp << "\') \n";
                         }
                      else ++n_def;
                     }
                status = ST_ENDLINE;
                break;

            case ST_ENDLINE : // Check the remaining after a macro definition
                while(c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f') get(c,fin); // Skip final spaces
                // Now see what we have
                if(c=='\n') { ++l; status=ST_NEWLINE; } // Detect expected line break
                else if( c=='/' && fin.peek()=='/' ) status = ST_SKIPLINE; // A comment
                else {// Garbage
                      ++issues;
                      std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_ENDLINE)\n";
                      //char s[256]; fin.getline(s,256); std::cerr << s << std::endl;
                      status = ST_SKIPLINE; // Garbage, skip the line
                     }
                get(c,fin); // Next
                break;

           } // 'switch(status)'
       } // 'while(!EOF)'
    // Finally
    std::cout << "  Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << "\n";
    return issues;
} // 'Parse_H'

//---------------------------------------------------------------------------
// A parser for Fagor 'DEF' files
template<typename T,bool E=true> int Parse_D(cls_Dictionary& dict, std::istream& fin)
{
    fun_ReadChar<T,E> get;
    std::cout << "  Parsing as: Fagor DEF file\n";

    enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEF, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
    int issues=0; // Number of issues of the parsing
    int n_def=0; // Number of encountered defines
    int l=1; // Current line number
    std::string def,exp;
    T c;
    get(c,fin);
    while( fin )
       {
        // According to current status
        switch( status )
           {
            case ST_SKIPLINE : // Skip current line
                if( c=='\n' )
                   {
                    ++l;
                    status = ST_NEWLINE;
                   }
                get(c,fin); // Next
                break;

            case ST_NEWLINE : // See what we have
                while(c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f') get(c,fin); // Skip initial spaces
                // Now see what we have
                if( c=='D' ) status = ST_DEF; // could be a 'DEF'
                else if( c==';' ) status = ST_SKIPLINE; // A Fagor comment
                else if( c=='\n' ) ++l; // Detect possible line break (empty line)
                else {// Garbage
                      ++issues;
                      std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_NEWLINE)\n";
                      status = ST_SKIPLINE; // Garbage, skip the line
                     }
                get(c,fin); // Next
                break;

            case ST_DEF : // Detect 'DEF' directive
                status = ST_SKIPLINE; // Default: garbage, skip the line
                if(              c=='E')
                if(get(c,fin) && c=='F')
                   {// Well now should have a space
                    get(c,fin);
                    if(c==' ' || c=='\t')
                       {// Got a define
                        status = ST_MACRO;
                        get(c,fin); // Next
                       }
                   }
                // Notify garbage
                if(status==ST_SKIPLINE)
                   {// Garbage
                    ++issues;
                    std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_DEF)\n";
                   }
                break;

            case ST_MACRO : // Collect the define macro string
                while(c==' ' || c=='\t') get(c,fin); // Skip spaces
                // Collect the macro string
                def = "";
                // The token ends with control chars or eof
                while( c!=EOF && c>' ' )
                   {
                    def += c;
                    get(c,fin); // Next
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
                while(c==' ' || c=='\t') get(c,fin); // Skip spaces
                // Collect the expansion string
                exp = "";
                // The token ends with control chars or eof
                while( c!=EOF && c>' ' )
                   {
                    exp += c;
                    get(c,fin); // Next
                   }
                if( exp.empty() )
                     {// No expansion defined!
                      ++issues;
                      std::cerr << "  No expansion defined in line " << l << std::endl;
                     }
                else {
                      // Check special syntax for PLC resources: ([R|M|I|O])(\d+)
                      // ex. R123 ==> V.PLC.R[123]    B5R123 ==> [V.PLC.R[123]&2**5]
                      std::string prfx;
                      auto p = exp.find(prfx="R");
                      // TODO 5: this is not so efficient, but more appealing to eyes
                      if(p!=0) p = exp.find(prfx="M");
                      if(p!=0) p = exp.find(prfx="I");
                      if(p!=0) p = exp.find(prfx="O");
                      if(p!=0) p = exp.find(prfx="LI");
                      //...if(p!=0) p = exp.find(prfx="LO");
                      if(p==0)
                         {
                          std::string idx = exp.substr( prfx.length() );
                          if( mat::is_index(idx) )
                             {
                              exp = "V.PLC." + prfx + "[" + idx + "]";
                             }
                         }
                      // Insert in dictionary
                      auto ins = dict.insert( cls_Dictionary::value_type( def, exp ) );
                      if( !ins.second )
                         {
                          ++issues;
                          std::cerr << "  \'" << ins.first->first << "\' already existed in line " << l << " (was \'" << ins.first->second << "\', now \'" << exp << "\') \n";
                         }
                      else ++n_def;
                     }
                status = ST_ENDLINE;
                break;

            case ST_ENDLINE : // Check the remaining after a macro definition
                while(c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f') get(c,fin); // Skip final spaces
                // Now see what we have
                if(c=='\n') { ++l; status=ST_NEWLINE; } // Detect expected line break
                else if( c==';' ) status = ST_SKIPLINE; // A comment
                else {// Garbage
                      ++issues;
                      std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_ENDLINE)\n";
                      //char s[256]; fin.getline(s,256); std::cerr << s << std::endl;
                      status = ST_SKIPLINE; // Garbage, skip the line
                     }
                get(c,fin); // Next
                break;

           } // 'switch(status)'
       } // 'while(!EOF)'
    // Finally
    std::cout << "  Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << "\n";
    return issues;
} // 'Parse_D'



//---------------------------------------------------------------------------
// A simple fast parser for:
// #define XXX YYY // comment
// DEF XXX YYY ; comment
int cls_Dictionary::LoadFile( const std::string& pth )
{
    // (0) Open file for read
    std::cout << std::endl << '[' << pth << ']' << std::endl;
    std::ifstream fin( pth, std::ios::binary );
    if( !fin )
       {
        std::cerr << "  Cannot read the file!\n";
        return 1;
       }

    // (1) See syntax (extension) and parse
    std::string dir,nam,ext;
    mat::split_path(pth, dir,nam,ext);
    bool fagor = (mat::tolower(ext)==".plc");

    // (2) Parse the file
    EN_ENCODING enc = mat::CheckBOM( fin );
    // Get the rest
    if( enc==ANSI || enc==UTF8 )
         {
          if(fagor) return Parse_D<char>(*this, fin);
          else return Parse_H<char>(*this, fin);
         }
    else if( enc==UTF16_LE )
         {
          if(fagor) return Parse_D<char16_t,true>(*this, fin);
          return Parse_H<char16_t,true>(*this, fin);
         }
    else if( enc==UTF16_BE )
         {
          if(fagor) return Parse_D<char16_t,false>(*this, fin);
          return Parse_H<char16_t,false>(*this, fin);
         }
    //else if( enc==UTF32_LE )
    //     {
    //      if(fagor) return Parse_D<char32_t,true>(*this, fin);
    //      return Parse_H<char32_t,true>(*this, fin);
    //     }
    //else if( enc==UTF32_BE )
    //     {
    //      if(fagor) return Parse_D<char32_t,false>(*this, fin);
    //      return Parse_H<char32_t,false>(*this, fin);
    //     }
    else {
          std::cerr << "  Cannot handle this encoding!\n";
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
                std::cerr << "\'" << ret.first->first << "\' already existed (was \'" << ret.first->second << "\') \n";
                ++issues;
               }
           }
       }
    fin.close();

    // (3) Finally
    return issues;
} // 'LoadFile'
*/



