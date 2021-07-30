#ifndef process_hpp
#define process_hpp
/*  ---------------------------------------------
    Process a file tokenizing and substituting
    dictionary
    --------------------------------------------- */
    #include "system.hpp" // 'sys::', 'fs::'
    #include <string>
    #include <map>
    #include <fstream> // 'std::ifstream'
    #include <iostream> // 'std::cerr'


//---------------------------------------------------------------------------
// Process a file (ANSI 8bit) tokenizing and substituting dictionary
// TODO 3: manage inlined directives: #define, #ifdef, #else, #endif
int process( const std::string& pth_in,
             const std::string& pth_out,
             const std::map<std::string,std::string>& dict,
             const bool overwrite,
             const bool verbose )
{
    // (0) See file name
    if( pth_in==pth_out )
       {
        notify_error("Same output file name {}", pth_in);
       }
    // Check extension?
    //std::string dir_in,nam_in,ext_in;
    //mat::split_path(pth_in, dir_in,nam_in,ext_in);
    //bool fagor_symb = (enc::tolower(ext_in)==".ncs");

    // (1) Open input file for read
    std::cout << "\n " << pth_in << " >>\n";
    std::ifstream fin( pth_in, std::ios::binary );
    if( !fin )
       {
        notify_error("Cannot read the file {}", pth_in);
       }

    // (2) Open output file for write
    std::cout << "  >> " << pth_out << '\n';
    // Check overwrite flag
    if( !overwrite && fs::exists(pth_out) )
       {// If must not overwrite, check existance
        std::ifstream fout(pth_out);
        if( fout.good() )
           {
            notify_error("Output file already existing (use -f to force overwrite)");
           }
       }

    //sys::file_write of( pth_out );
    //of << ...;

    std::ofstream fout( pth_out, std::ios::binary ); // Overwrite
    if( !fout )
       {
        notify_error("Cannot write to {}", pth_out);
       }

    // (3) Parse the file
    enum{ ST_SKIPLINE, ST_SEEK, ST_COLLECT } status = ST_SEEK;
    int n_tok=0, n_sub=0; // Number of encountered tokens and substitutions
    int l = 1; // Current line number
    std::string tok; // Bag for current token
    bool skipsub = false; // Auxiliary to handle '#' for skipping substitution
    unsigned int sqbr_opened = 0; // Auxiliary to handle square brackets in token
    // TODO 5: should deal with encoding, now supporting just 8bit
    enc::Bom bom(fin);
    if( !bom.is_ansi() )
       {
        // TODO 1: should manage other encodings
        notify_error("Cannot handle encoding {}", bom.to_str());
       }
    // Get the rest
    constexpr char eof = std::char_traits<char>::eof();
    char c = fin.get();
    while( c != eof )
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
                while( c!=eof && c>' ' &&
                       c!='+' && c!='-' && c!='*' && c!='=' &&
                       c!='(' && c!=')' && // c!='[' && c!=']' && (can be part of the token)
                       c!='{' && c!='}' && c!='<' && c!='>' &&
                       c!='!' && c!='&' && c!='|' && c!='^' &&
                       c!=':' && c!=',' && c!=';' && // c!='.' && (can be part of the token)
                       c!='/' && // <comment chars>
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
       } // 'while(!eof)'

    // (4) Finally
    //fout.flush(); // Ensure to write the disk
    if(verbose) std::cout << "  Expanded " << n_sub << " macros checking a total of " << n_tok << " tokens in " << l << " lines\n";
    return 0;
}


//---- end unit -------------------------------------------------------------
#endif
