#ifndef parse_hpp
#define parse_hpp
/*  ---------------------------------------------
    Parsing
    --------------------------------------------- */
    #include "logging.hpp" // 'dlg::parse_error', 'dlg::error', 'DBGLOG', 'fmt::format'
    #include "poor-mans-unicode.hpp" // 'enc::Bom'
    #include <string>
    #include <vector>
    #include <map>
    #include <istream> // 'std::istream'
    #include <cctype> // 'std::isspace', 'std::isblank', 'std::isdigit'
    //#include <regex> // 'std::regex_iterator'

    #include <fstream> // 'std::ifstream'



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace def //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
// A parser for Fagor 'DEF' files
// DEF macro expansion ; comment
template<typename T,bool LE> void parse(std::istream& is, std::map<std::string,std::string>& dict, std::vector<std::string>& issues, const bool fussy)
{
    enc::ReadChar<LE,T> get;

    enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEF, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
    int n_def=0; // Number of encountered defines
    int l=1; // Current line number
    std::basic_string<T> def, exp;
    constexpr T eof = std::char_traits<T>::eof();
    T c;
    get(c,is);
    while( is )
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
                get(c,is); // Next
                break;

            case ST_NEWLINE : // See what we have
                while( std::isspace(c) && c!='\n' ) get(c,is); // Skip initial spaces
                // Now see what we have
                if( c=='D' ) status = ST_DEF; // could be a 'DEF'
                else if( c==';' ) status = ST_SKIPLINE; // A Fagor comment
                else if( c=='\n' ) ++l; // Detect possible line break (empty line)
                else
                   {// Garbage
                    throw dlg::error("Unexpected content \'{}\' in line {} (ST_NEWLINE)\n", c, l);
                    status = ST_SKIPLINE; // Garbage, skip the line
                   }
                get(c,is); // Next
                break;

            case ST_DEF : // Detect 'DEF' directive
                status = ST_SKIPLINE; // Default: garbage, skip the line
                if(             c=='E')
                if(get(c,is) && c=='F')
                   {// Well now should have a space
                    get(c,is);
                    if( std::isblank(c) )
                       {// Got a define
                        status = ST_MACRO;
                        get(c,is); // Next
                       }
                   }
                // Notify garbage
                if(status==ST_SKIPLINE)
                   {// Garbage
                    throw dlg::error("Unexpected content \'{}\' in line {} (ST_DEF)", c, l);
                   }
                break;

            case ST_MACRO : // Collect the define macro string
                while( std::isblank(c) ) get(c,is); // Skip spaces
                // Collect the macro string
                def.clear();
                // The token ends with control chars or EOF
                while( c!=eof && c>' ' )
                   {
                    def += c;
                    get(c,is); // Next
                   }
                if( def.empty() )
                   {// No macro defined!
                    throw dlg::error("No macro defined in line {}", l);
                    status = ST_ENDLINE;
                   }
                else status = ST_EXP;
                break;

            case ST_EXP : // Collect the macro expansion string
               {
                while( std::isblank(c) ) get(c,is); // Skip spaces
                // Collect the expansion string
                exp.clear();
                std::basic_string<char> reg, idx; // TODO: should use T, but have problems with literals

                enum{ SST_FIRSTCHAR, SST_GENERIC, SST_REGIDX, SST_BITIDX, SST_BITREG, SST_DONE } substs = SST_FIRSTCHAR;
                // Note: the token ends with control chars or EOF (c!=eof && c>' ')
                while( is && substs!=SST_DONE )
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
                                    get(c,is); // Next
                                   }
                                else
                                   {// Not a register
                                    exp += c;
                                    get(c,is); // Next
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
                                    get(c,is); // Next
                                   }
                                else if( c=='R' )
                                   {// OK, now get the register index
                                    exp += c;
                                    get(c,is); // Next
                                    substs = SST_BITREG;
                                    break;
                                   }
                                else
                                   {// Not a bit notation
                                    exp += c;
                                    get(c,is); // Next
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
                                    get(c,is); // Next
                                   }
                                else
                                   {// Not a bit notation
                                    exp += c;
                                    get(c,is); // Next
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
                                get(c,is); // Next
                               }
                            substs = SST_DONE;
                            break;

                        case SST_FIRSTCHAR : // See first character
                            if( c=='M' || c=='R' || c=='I' || c=='O' )
                               {// Could be a Fagor register ex. M5100
                                reg = c;
                                exp += c;
                                get(c,is); // Next
                                substs = SST_REGIDX;
                               }
                            else if( c=='L' && is.peek()=='I' )
                               {// Could be a 'LI' Fagor register ex. LI1
                                reg = "LI";
                                exp += c; // 'L'
                                get(c,is); // Next
                                exp += c; // 'I'
                                get(c,is); // Next
                                substs = SST_REGIDX;
                               }
                            else if( c=='B' )
                               {// Could be a Fagor register bit notation ex. B5R100
                                exp += c;
                                get(c,is); // Next
                                substs = SST_BITIDX;
                               }
                            else if(c!=eof && c>' ')
                               {// A generic string expansion ex. 21000
                                exp += c;
                                get(c,is); // Next
                                substs = SST_GENERIC;
                               }
                            else
                               {// Token ended prematurely!?
                                //if(verbose) throw dlg::error("Token ended prematurely in line {}", l);
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
                      throw dlg::error("No expansion defined in line {}", l);
                     }
                else {// Insert in dictionary
                      auto ins = dict.insert( dict_t::value_type( def, exp ) );
                      if( !ins.second && ins.first->second!=exp )
                         {
                          throw dlg::error("\'{}\' redefined in line {} (was \'{}\', now \'{}\')", ins.first->first, l, ins.first->second, exp);
                         }
                      else ++n_def;
                     }
                status = ST_ENDLINE;
               } break;

            case ST_ENDLINE : // Check the remaining after a macro definition
                while( std::isspace(c) && c!='\n' ) get(c,is); // Skip final spaces
                // Now see what we have
                if(c=='\n') { ++l; status=ST_NEWLINE; } // Detect expected line break
                else if( c==';' ) status = ST_SKIPLINE; // A comment
                else
                   {// Garbage
                    throw dlg::error("Unexpected content \'" << c << "\' in line " << l << " (ST_ENDLINE)\n";
                    //char s[256]; is.getline(s,256); std::cerr << s << '\n';
                    status = ST_SKIPLINE; // Garbage, skip the line
                   }
                get(c,is); // Next
                break;

           } // 'switch(status)'
       } // 'while(!eof)'
    //std::cout << "Collected " << n_def << " defines in " << l << " lines, overall dict size: " << dict.size() << "\n";
}



/*
//---------------------------------------------------------------------------
// Using regular expression
void parse( const std::string& pth )
{
    std::regex regdef( R"(^\s*(?:#define|DEF)\s+(\S+)\s+(\S+)(?:\s+(?:;|//)\s*([^\n]+))?)" );

    // Open file for read
    std::ifstream is( pth );
    if( !is )
       {
        throw dlg::error("Cannot open {}",pth);
        return 1;
       }

    // Scan lines
    std::string line;
    while( std::getline(is, line) )
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
                throw dlg::error("\'{}\' redefined in line {} (was \'{}\', now \'{}\', ins.first->first, l, ins.first->second, rit->str(2));
               }
           }
       }
    is.close();
}
*/


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif