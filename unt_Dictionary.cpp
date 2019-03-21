//#include <stdexcept> // 'std::runtime_error'
#include <cassert> // 'assert'
#include <iostream> // 'std::cerr'
#include <fstream> // 'std::ifstream'
#include <cctype> // 'std::isspace', 'std::isblank', 'std::isdigit'
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
void cls_Dictionary::Invert(const bool nonum, const bool verbose)
{
    if(verbose) std::cout << "\n{Inverting dictionary}\n";
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
              //std::cerr << "  Adding alias \'" << i->first << "\' for " << i->second << '\n';
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
   if(verbose) std::cout << "  Now got " << inv_map.size() << " voices from previous " << size() << '\n';
   // Finally, assign the inverted dictionary
   inherited::operator=( inv_map );
} // 'Invert'


//---------------------------------------------------------------------------
// Debug utility
void cls_Dictionary::Peek()
{
    int max = 200;
    for(auto i=begin(); i!=end(); ++i)
       {
        std::cout << "  " << i->first << " " << i->second << '\n';
        if(--max<0) { std::cout << "...\n"; break; }
       }
} // 'Peek'




/////////////////////////////////////////////////////////////////////////////
// A function object that read a char from byte stream
template<typename T,bool E=true> class fun_ReadChar /////////////////////////
{
 public:
    fun_ReadChar(const bool verbose)
       {
        if(verbose) std::cout << "  Reading stream unit: " << sizeof(T) << " bytes\n";
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
    fun_ReadChar(const bool verbose)
       {
        if(verbose) std::cout << "  Reading stream unit: char32_t little endian\n";
       }
    inline std::istream& operator()(char32_t& c, std::istream& in)
       {
        in.read(buf,4);
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
    fun_ReadChar(const bool verbose)
       {
        if(verbose) std::cout << "  Reading stream unit: char32_t big endian\n";
       }
    inline std::istream& operator()(char32_t& c, std::istream& in)
       {
        in.read(buf,4);
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
    fun_ReadChar(const bool verbose)
       {
        if(verbose) std::cout << "  Reading stream unit: char16_t little endian\n";
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
    fun_ReadChar(const bool verbose)
       {
        if(verbose) std::cout << "  Reading stream unit: char16_t big endian\n";
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
    fun_ReadChar(const bool verbose)
       {
        if(verbose) std::cout << "  Reading stream unit: char\n";
       }
    inline std::istream& operator()(char& c, std::istream& in)
       {
        return in.get(c);
       }
}; // 'fun_ReadChar'


//---------------------------------------------------------------------------
// A parser for c-like '#define' directives
template<typename T,bool E=true> int Parse_H(cls_Dictionary& dict, std::istream& fin, const bool verbose =false)
{
    fun_ReadChar<T,E> get(verbose);
    if(verbose) std::cout << "  Parsing as: C-like defines\n";
    
    enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEFINE, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
    int issues=0; // Number of issues of the parsing
    int n_def=0; // Number of encountered defines
    int l=1; // Current line number
    std::basic_string<char> def,exp;
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
                while( std::isspace(c) && c!='\n' ) get(c,fin); // Skip initial spaces
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
                    if( std::isblank(c) )
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
                while( std::isblank(c) ) get(c,fin); // Skip spaces
                // Collect the macro string
                def = "";
                // The token ends with control chars or EOF
                while( c!=EOF && c>' ' )
                   {
                    // if(c=='/') invalid macro chars
                    // TODO: dovrei convertirlo in UTF-8
                    def += c;
                    get(c,fin); // Next
                   }
                if( def.empty() )
                     {// No macro defined!
                      ++issues;
                      std::cerr << "  No macro defined in line " << l << '\n';
                      status = ST_ENDLINE;
                     }
                else status = ST_EXP;
                break;

            case ST_EXP : // Collect the macro expansion string
                while( std::isblank(c) ) get(c,fin); // Skip spaces
                // Collect the expansion string
                exp = "";
                // The token ends with control chars or EOF
                // ...or get all, until comment, EOL or EOF
                while( c!=EOF && c>' ' )
                   {
                    // TODO: dovrei convertirlo in UTF-8
                    exp += c;
                    get(c,fin); // Next
                   }
                if( exp.empty() )
                     {// No expansion defined!
                      ++issues;
                      std::cerr << "  No expansion defined in line " << l << '\n';
                     }
                else {
                      // Insert in dictionary
                      auto ins = dict.insert( cls_Dictionary::value_type( def, exp ) );
                      if( !ins.second && ins.first->second!=exp )
                         {
                          ++issues;
                          std::cerr << "  \'" << ins.first->first << "\' redefined in line " << l << " (was \'" << ins.first->second << "\', now \'" << exp << "\') \n";
                         }
                      else ++n_def;
                     }
                status = ST_ENDLINE;
                break;

            case ST_ENDLINE : // Check the remaining after a macro definition
                while( std::isspace(c) && c!='\n' ) get(c,fin); // Skip final spaces
                // Now see what we have
                if(c=='\n') { ++l; status=ST_NEWLINE; } // Detect expected line break
                else if( c=='/' && fin.peek()=='/' ) status = ST_SKIPLINE; // A comment
                else {// Garbage
                      ++issues;
                      std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_ENDLINE)\n";
                      //char s[256]; fin.getline(s,256); std::cerr << s << '\n';
                      status = ST_SKIPLINE; // Garbage, skip the line
                     }
                get(c,fin); // Next
                break;

           } // 'switch(status)'
       } // 'while(!EOF)'
    // Finally
    if(verbose) std::cout << "  Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << "\n";
    return issues;
} // 'Parse_H'

//---------------------------------------------------------------------------
// A parser for Fagor 'DEF' files
template<typename T,bool E=true> int Parse_D(cls_Dictionary& dict, std::istream& fin, const bool verbose =false)
{
    fun_ReadChar<T,E> get(verbose);
    if(verbose) std::cout << "  Parsing as: Fagor DEF file\n";

    enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEF, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
    int issues=0; // Number of issues of the parsing
    int n_def=0; // Number of encountered defines
    int l=1; // Current line number
    std::string def, exp;
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
                while( std::isspace(c) && c!='\n' ) get(c,fin); // Skip initial spaces
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
                    if( std::isblank(c) )
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
                while( std::isblank(c) ) get(c,fin); // Skip spaces
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
                      std::cerr << "  No macro defined in line " << l << '\n';
                      status = ST_ENDLINE;
                     }
                else status = ST_EXP;
                break;

            case ST_EXP : // Collect the macro expansion string
               {
                while( std::isblank(c) ) get(c,fin); // Skip spaces
                // Collect the expansion string
                exp = "";
                std::string reg, idx;

                enum{ SST_FIRSTCHAR, SST_GENERIC, SST_REGIDX, SST_BITIDX, SST_BITREG, SST_DONE } substs = SST_FIRSTCHAR;
                // Note: the token ends with control chars or eof (c!=EOF && c>' ')
                while( fin && substs!=SST_DONE )
                   {
                    // According to current substatus
                    switch( substs )
                       {
                        case SST_REGIDX : // Collecting register index
                            while( c!=EOF && c>' ' )
                               {
                                if( std::isdigit(c) )
                                     {
                                      idx += c;
                                      exp += c;
                                      get(c,fin); // Next
                                     }
                                else {// Not a register
                                      exp += c;
                                      get(c,fin); // Next
                                      substs = SST_GENERIC;
                                      break;
                                     }
                               }
                            if(substs==SST_REGIDX)
                               {// Collected a Fagor register!
                                // I'll convert this special syntax
                                // ex. R123 ==> V.PLC.R[123]
                                exp = "V.PLC." + reg + "[" + idx + "]";
                                substs = SST_DONE;
                               }
                            break;

                        case SST_BITIDX : // Collecting bit index
                            while( c!=EOF && c>' ' )
                               {
                                if( std::isdigit(c) )
                                     {
                                      idx += c;
                                      exp += c;
                                      get(c,fin); // Next
                                     }
                                else if( c=='R' )
                                     {// OK, now get the register index
                                      exp += c;
                                      get(c,fin); // Next
                                      substs = SST_BITREG;
                                      break;
                                     }
                                else {// Not a bit notation
                                      exp += c;
                                      get(c,fin); // Next
                                      substs = SST_GENERIC;
                                      break;
                                     }
                               }
                            break;

                        case SST_BITREG : // Collecting register of a bit notation
                            while( c!=EOF && c>' ' )
                               {
                                if( std::isdigit(c) )
                                     {
                                      reg += c;
                                      exp += c;
                                      get(c,fin); // Next
                                     }
                                else {// Not a bit notation
                                      exp += c;
                                      get(c,fin); // Next
                                      substs = SST_GENERIC;
                                      break;
                                     }
                               }
                            if(substs==SST_BITREG)
                               {// Collected a Fagor bit notation!
                                // I'll convert this special syntax
                                // ex. B5R123 ==> [V.PLC.R[123]&2**5]
                                exp = "[V.PLC.R[" + reg + "]&2**" + idx + "]";
                                substs = SST_DONE;
                               }
                            break;

                        case SST_GENERIC : // Collecting generic macro
                            while( c!=EOF && c>' ' )
                               {
                                exp += c;
                                get(c,fin); // Next
                               }
                            substs = SST_DONE;
                            break;

                        case SST_FIRSTCHAR : // See first character
                            if( c=='M' || c=='R' || c=='I' || c=='O' )
                                 {// Could be a Fagor register ex. M5100
                                  reg = c;
                                  exp += c;
                                  get(c,fin); // Next
                                  substs = SST_REGIDX;
                                 }
                            else if( c=='L' && fin.peek()=='I' )
                                 {// Could be a 'LI' Fagor register ex. LI1
                                  reg = "LI";
                                  exp += c; // 'L'
                                  get(c,fin); // Next
                                  exp += c; // 'I'
                                  get(c,fin); // Next
                                  substs = SST_REGIDX;
                                 }
                            else if( c=='B' )
                                 {// Could be a Fagor register bit notation ex. B5R100
                                  exp += c;
                                  get(c,fin); // Next
                                  substs = SST_BITIDX;
                                 }
                            else if(c!=EOF && c>' ')
                                 {// A generic string expansion ex. 21000
                                  exp += c;
                                  get(c,fin); // Next
                                  substs = SST_GENERIC;
                                 }
                            else {// Token ended prematurely!?
                                  if(verbose) std::cerr << "  Token ended prematurely in line " << l << '\n';
                                  substs = SST_DONE;
                                 }
                            break;

                        case SST_DONE : break;
                       } // collecting expansion string
                   } // while stream ok

                // Use collected expansion string
                if( exp.empty() )
                     {// No expansion defined!
                      ++issues;
                      std::cerr << "  No expansion defined in line " << l << '\n';
                     }
                else {// Insert in dictionary
                      auto ins = dict.insert( cls_Dictionary::value_type( def, exp ) );
                      if( !ins.second && ins.first->second!=exp )
                         {
                          ++issues;
                          std::cerr << "  \'" << ins.first->first << "\' redefined in line " << l << " (was \'" << ins.first->second << "\', now \'" << exp << "\') \n";
                         }
                      else ++n_def;
                     }
                status = ST_ENDLINE;
               } break;

            case ST_ENDLINE : // Check the remaining after a macro definition
                while( std::isspace(c) && c!='\n' ) get(c,fin); // Skip final spaces
                // Now see what we have
                if(c=='\n') { ++l; status=ST_NEWLINE; } // Detect expected line break
                else if( c==';' ) status = ST_SKIPLINE; // A comment
                else {// Garbage
                      ++issues;
                      std::cerr << "  Unexpected content \'" << c << "\' in line " << l << " (ST_ENDLINE)\n";
                      //char s[256]; fin.getline(s,256); std::cerr << s << '\n';
                      status = ST_SKIPLINE; // Garbage, skip the line
                     }
                get(c,fin); // Next
                break;

           } // 'switch(status)'
       } // 'while(!EOF)'
    // Finally
    if(verbose) std::cout << "  Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << "\n";
    return issues;
} // 'Parse_D'



//---------------------------------------------------------------------------
// A simple fast parser for:
// #define XXX YYY // comment
// DEF XXX YYY ; comment
int cls_Dictionary::LoadFile( const std::string& pth, const bool verbose )
{
    // (0) Open file for read
    if(verbose) std::cout << "\n[" << pth << "]\n";
    std::ifstream fin( pth, std::ios::binary );
    if( !fin )
       {
        std::cerr << "  Cannot read " << pth << '\n';
        return 1;
       }

    // (1) See syntax (extension) and parse
    std::string dir,nam,ext;
    mat::split_path(pth, dir,nam,ext);
    bool fagor = (mat::tolower(ext)==".plc");

    // (2) Parse the file
    EN_ENCODING enc = mat::CheckBOM( fin, nullptr, verbose );
    // Get the rest
    if( enc==ANSI || enc==UTF8 )
         {
          if(fagor) return Parse_D<char>(*this, fin, verbose);
          else return Parse_H<char>(*this, fin, verbose);
         }
    else if( enc==UTF16_LE )
         {
          if(fagor) return Parse_D<char16_t,true>(*this, fin, verbose);
          return Parse_H<char16_t,true>(*this, fin, verbose);
         }
    else if( enc==UTF16_BE )
         {
          if(fagor) return Parse_D<char16_t,false>(*this, fin, verbose);
          return Parse_H<char16_t,false>(*this, fin, verbose);
         }
    //else if( enc==UTF32_LE )
    //     {
    //      if(fagor) return Parse_D<char32_t,true>(*this, fin, verbose);
    //      return Parse_H<char32_t,true>(*this, fin, verbose);
    //     }
    //else if( enc==UTF32_BE )
    //     {
    //      if(fagor) return Parse_D<char32_t,false>(*this, fin, verbose);
    //      return Parse_H<char32_t,false>(*this, fin, verbose);
    //     }
    else {
          std::cerr << "  Cannot handle the encoding of " << pth << "\n";
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
        std::cerr << "!! Cannot open " << pth << '\n';
        return 1;
       }

    // (2) Scan lines
    int issues = 0;
    std::string line;
    while( std::getline(fin, line) )
       {
        //std::stringstream lineStream(line);
        //std::string token;
        //while(lineStream >> token) std::cout << "Token: " << token << '\n';

        std::regex_iterator<std::string::iterator> rit ( line.begin(), line.end(), regdef ), rend;
        //while( rit!=rend ) { std::cout << rit->str() << '\n'; ++rit; }
        if( rit!=rend )
           {
            //std::cout << '\n' for(size_type i=0; i<rit->size(); ++i) std::cout << rit->str(i) << '\n';
            assert( rit->size()==3 );
            auto ins = insert( value_type( rit->str(1), rit->str(2) ) );
            if( !ins.second && ins.first->second!=exp )
               {
                std::cerr << "  \'" << ins.first->first << "\' redefined in line " << l << " (was \'" << ins.first->second << "\', now \'" << rit->str(2) << "\') \n";
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
// Process a file (ANSI 8bit) tokenizing and substituting dictionary
// TODO 3: manage inlined directives: #define, #ifdef, #else, #endif
int cls_Dictionary::Process( const std::string& pth_in,
                             const std::string& pth_out,
                             const cls_Dictionary& dict,
                             const bool overwrite,
                             const bool verbose,
                             const char cmtchar )
{
    // (0) See file name
    if( pth_in==pth_out )
       {
        std::cerr << "  Same output file name! " << pth_in << '\n';
        return 1;
       }
    // Check extension?
    //std::string dir_in,nam_in,ext_in;
    //mat::split_path(pth_in, dir_in,nam_in,ext_in);
    //bool fagor_symb = (mat::tolower(ext_in)==".ncs");

    // (1) Open input file for read
    std::cout << "\n " << pth_in << " >>\n";
    std::ifstream fin( pth_in, std::ios::binary );
    if( !fin )
       {
        std::cerr << "  Cannot read the file!\n";
        return 1;
       }

    // (2) Open output file for write
    std::cout << "  >> " << pth_out << '\n';
    // Check overwrite flag
    if( !overwrite )
       {// If must not overwrite, check existance
        std::ifstream fout(pth_out);
        if( fout.good() )
           {
            std::cerr << "  Output file already existing!! (use -f to force overwrite)\n";
            return 1;
           }
       }
    std::ofstream fout( pth_out, std::ios::binary ); // Overwrite
    if( !fout )
       {
        std::cerr << "  Cannot write the output file " << pth_out << "\n";
        return 1;
       }

    // (3) Parse the file
    enum{ ST_SKIPLINE, ST_SEEK, ST_COLLECT } status = ST_SEEK;
    int n_tok=0, n_sub=0; // Number of encountered tokens and substitutions
    int l = 1; // Current line number
    std::string tok; // Bag for current token
    bool skipsub = false; // Auxiliary to handle '#' for skipping substitution
    unsigned int sqbr_opened = 0; // Auxiliary to handle square brackets in token
    // TODO 5: should deal with encoding, now supporting just 8bit enc
    EN_ENCODING enc = mat::CheckBOM( fin, &fout, verbose );
    if( enc != ANSI )
       {
        // TODO 1: should manage other encodings
        std::cerr << "  Cannot handle this encoding!\n";
        return 1;
       }
    // Get the rest
    char c = fin.get();
    while( c != EOF )
       {
        //std::cout << c << " line: " << l << " tok:" << tok << " status: " << status << '\n';
        // According to current status
        switch( status )
           {
            case ST_SKIPLINE : // Skip comment line
                fout << c;
                if( c=='\n' )
                   {
                    ++l;
                    status = ST_SEEK;
                   }
                c = fin.get();
                break;

            case ST_SEEK : // Seek next token
                if( c=='\n' )
                     {// Detect possible line break
                      fout << c;
                      ++l;
                      c=fin.get();
                     }
                else if( c<=' ' )
                     {// Skip control characters
                      fout << c;
                      c = fin.get();
                     }
                else if( c=='/' )
                     {// Skip division/detect c++ comment line
                      fout << c;
                      c = fin.get();
                      if( c=='/' )
                         {
                          fout << c;
                          c = fin.get();
                          status = ST_SKIPLINE;
                         }
                     }
                else if( c==cmtchar )
                     {// Skip line comment char
                      fout << c;
                      c = fin.get();
                      status = ST_SKIPLINE;
                     }
                else if( c=='+' || c=='-' || c=='*' || c=='=' ||
                         c=='(' || c==')' || c=='[' || c==']' ||
                         c=='{' || c=='}' || c=='<' || c=='>' ||
                         c=='!' || c=='&' || c=='|' || c=='^' ||
                         c==':' || c==',' || c=='.' || c==';' ||
                         // c!=cmtchar || c!='/' ||  // <comment chars>
                         c=='\'' || c=='\"' || c=='\\' ) //
                     {// Skip operators
                      fout << c;
                      c = fin.get();
                     }
                else {// Got a token: initialize status to get a new one
                      sqbr_opened = 0;
                      // Handle the 'no-substitution' character
                      if( c=='#' )
                           {
                            skipsub = true;
                            tok = "";
                           }
                      else {
                            skipsub = false;
                            tok = c;
                           }
                      c = fin.get(); // Next
                      status = ST_COLLECT;
                     }
                break;

            case ST_COLLECT : // Collect the rest of the token
                // The token ends with control chars or operators
                while( c!=EOF && c>' ' &&
                       c!='+' && c!='-' && c!='*' && c!='=' &&
                       c!='(' && c!=')' && // c!='[' && c!=']' && (can be part of the token)
                       c!='{' && c!='}' && c!='<' && c!='>' &&
                       c!='!' && c!='&' && c!='|' && c!='^' &&
                       c!=':' && c!=',' && c!=';' && // c!='.' && (can be part of the token)
                       c!=cmtchar && c!='/' && // <comment chars>
                       c!='\'' && c!='\"' && c!='\\' )
                   {
                    if(c=='[')
                       {
                        ++sqbr_opened;
                        // Se dentro quadre non trovo subito un numero non è un token
                        if( !std::isdigit(fin.peek()) ) break;
                       }
                    else if(c==']')
                       {
                        if(sqbr_opened>0) --sqbr_opened;
                        else break; // closing a not opened '['!!
                       }
                    // If here, collect token new character
                    tok += c;
                    c = fin.get(); // Next
                    if( sqbr_opened==0 && c==']' ) break; // Detect square bracket close
                   }

                // Use token
                ++n_tok;
                // See if it's a defined macro
                auto def = dict.find(tok);
                if( def != dict.end() )
                     {// Got a macro
                      if(skipsub)
                           {// Don't substitute, leave out the '#'
                            fout << tok;
                           }
                      else {// Substitute
                            fout << def->second;
                            ++n_sub;
                           }
                     }
                else {// Not a define, pass as it is
                      if(skipsub) fout << '#'; // Wasn't a define, re-add the '#'
                      fout << tok;
                     }

                // Finally
                status = ST_SEEK;
                break;

           } // 'switch(status)'
       } // 'while(!EOF)'

    // (4) Finally
    //fout.flush(); // Ensure to write the disk
    if(verbose) std::cout << "  Expanded " << n_sub << " macros checking a total of " << n_tok << " tokens in " << l << " lines\n";
    return 0;
} // 'Process'


