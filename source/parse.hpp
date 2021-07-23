#ifndef parse_hpp
#define parse_hpp
/*  ---------------------------------------------
    Parsing
    --------------------------------------------- */
    //#include <stdexcept> // 'std::runtime_error'
    #include <string>
    #include <istream> // 'std::istream'
    #include <cctype> // 'std::isspace', 'std::isblank', 'std::isdigit'
    //#include <regex> // 'std::regex_iterator'
    #include <iostream> // 'std::cerr'
    #include "poor-mans-unicode.hpp"






//---------------------------------------------------------------------------
// A parser for c-like '#define' directives
// #define XXX YYY // comment
template<typename T,bool E=true> int Parse_H(dict_t& dict, std::istream& fin, const bool verbose =false)
{
    enc::ReadChar<T,E> get(verbose);
    if(verbose) std::cout << "  Parsing as: C-like defines\n";
    
    enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEFINE, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
    int issues=0; // Number of issues of the parsing
    int n_def=0; // Number of encountered defines
    int l=1; // Current line number
    std::basic_string<char> def,exp;
    constexpr T eof = std::char_traits<T>::eof();
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
                while( c!=eof && c>' ' )
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
                while( c!=eof && c>' ' )
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
                else
                   {
                    // Insert in dictionary
                    auto ins = dict.insert( dict_t::value_type( def, exp ) );
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
       } // 'while(!eof)'
    // Finally
    if(verbose) std::cout << "  Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << "\n";
    return issues;
} // 'Parse_H'

//---------------------------------------------------------------------------
// A parser for Fagor 'DEF' files
// DEF XXX YYY ; comment
template<typename T,bool E=true> int Parse_D(dict_t& dict, std::istream& fin, const bool verbose =false)
{
    enc::ReadChar<T,E> get(verbose);
    if(verbose) std::cout << "  Parsing as: Fagor DEF file\n";

    enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEF, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
    int issues=0; // Number of issues of the parsing
    int n_def=0; // Number of encountered defines
    int l=1; // Current line number
    std::basic_string<char> def, exp; // TODO: should use T, but have problems with literals
    constexpr T eof = std::char_traits<T>::eof();
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
                def.clear();
                // The token ends with control chars or EOF
                while( c!=eof && c>' ' )
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
                exp.clear();
                std::basic_string<char> reg, idx; // TODO: should use T, but have problems with literals

                enum{ SST_FIRSTCHAR, SST_GENERIC, SST_REGIDX, SST_BITIDX, SST_BITREG, SST_DONE } substs = SST_FIRSTCHAR;
                // Note: the token ends with control chars or EOF (c!=eof && c>' ')
                while( fin && substs!=SST_DONE )
                   {
                    // According to current substatus
                    switch( substs )
                       {
                        case SST_REGIDX : // Collecting register index
                            while( c!=eof && c>' ' )
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
                            while( c!=eof && c>' ' )
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
                            while( c!=eof && c>' ' )
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
                            while( c!=eof && c>' ' )
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
                            else if(c!=eof && c>' ')
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
                      auto ins = dict.insert( dict_t::value_type( def, exp ) );
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
       } // 'while(!eof)'
    // Finally
    if(verbose) std::cout << "  Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << "\n";
    return issues;
} // 'Parse_D'



/*
//---------------------------------------------------------------------------
// Using regular expression
int parse( const std::string& pth )
{
    // See extension
    //std::string ext( pth.find_last_of(".")!=std::string::npos ? pth.substr(pth.find_last_of(".")+1) : "" );
    //if( ext=="def" )
    std::regex regdef( R"(^\s*(?:#define|DEF)\s+(\S+)\s+(\S+))" );

    // Open file for read
    std::ifstream fin( pth );
    if( !fin )
       {
        std::cerr << "!! Cannot open " << pth << '\n';
        return 1;
       }

    // Scan lines
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

    return issues;
}
*/


//---- end unit -------------------------------------------------------------
#endif

