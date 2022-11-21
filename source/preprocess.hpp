#ifndef preprocess_hpp
#define preprocess_hpp
//  ---------------------------------------------
//  Process a file tokenizing and substituting
//  dictionary
//  ---------------------------------------------
#include "system.hpp" // sys::*, fs::*
#include <fstream> // std::*fstream
#include <string>
#include <map>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace pre //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

    struct process_result_t
    {
        std::size_t n_tok = 0; // Tokens counter
        std::size_t n_sub = 0; // Substitutions counter
        std::size_t n_line = 1; // Linea counter
    };


//---------------------------------------------------------------------------
// Process a file (ANSI 8bit) tokenizing and substituting dictionary
// TODO 3: manage inlined directives: #define, #ifdef, #else, #endif
process_result_t process( const fs::path& pth_in,
                          const fs::path& pth_out,
                          const std::map<std::string,std::string>& dict,
                          const bool overwrite )
{
    // Meglio evitare di sovrascrivere il file di input
    if( std::error_code ec;
        fs::equivalent(pth_in, pth_out, ec) )
       {
        throw dlg::error("Wont overwrite input {}", pth_in.string());
       }

    // Open input file for read
    std::ifstream fin( pth_in, std::ios::binary );
    if( !fin ) throw dlg::error("Cannot read the file {}", pth_in.string());
    //sys::MemoryMappedFile in_buf( pth.string() );
    //in_buf.as_string_view()

    // Open output file for write
    if( !overwrite && fs::exists(pth_out) ) throw dlg::error("Output file already existing (use -f to force overwrite)");
    std::ofstream fout( pth_out, std::ios::binary ); // Overwrite
    if( !fout ) throw dlg::error("Cannot write to {}", pth_out.string());
    //sys::file_write of( pth_out );
    //of << ...;

    // Parse the input file
    enum{ ST_SKIPLINE, ST_SEEK, ST_COLLECT } status = ST_SEEK;
    process_result_t res;
    std::string tok; // Bag for current token
    bool skipsub = false; // Auxiliary to handle '#' for skipping substitution
    unsigned int sqbr_opened = 0; // Auxiliary to handle square brackets in token

    // TODO 5: Should deal with encoding, now supporting just 8bit
    enc::Bom bom(fin);
    if( !bom.is_ansi() && !bom.is_utf8() )
       {
        throw dlg::error("Cannot handle encoding {}", bom.to_str());
       }

    // Get the rest
    constexpr char eof = std::char_traits<char>::eof();
    char c = fin.get();
    while( c != eof )
       {
        //std::cout << c << " line: " << res.n_line << " tok:" << tok << " status: " << status << '\n';
        // According to current status
        switch( status )
           {
            case ST_SKIPLINE : // Skip comment line
                fout << c;
                if( c=='\n' )
                   {
                    ++res.n_line;
                    status = ST_SEEK;
                   }
                c = fin.get();
                break;

            case ST_SEEK : // Seek next token
                if( c=='\n' )
                   {// Detect possible line break
                    fout << c;
                    ++res.n_line;
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
                //else if( c==cmtchar )
                //   {// Skip line comment char
                //    fout << c;
                //    c = fin.get();
                //    status = ST_SKIPLINE;
                //   }
                //else if( std::ispunct(c) )
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
                else
                   {// Got a token: initialize status to get a new one
                    sqbr_opened = 0;
                    // Handle the 'no-substitution' character
                    if( c=='#' )
                       {
                        skipsub = true;
                        tok = "";
                       }
                    else
                       {
                        skipsub = false;
                        tok = c;
                       }
                    c = fin.get(); // Next
                    status = ST_COLLECT;
                   }
                break;

            case ST_COLLECT : // Collect the rest of the token
                // The token ends with control chars or operators
                // !std::iscntrl(c) && !std::ispunct(c)
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
                ++res.n_tok;
                // See if it's a defined macro
                auto def = dict.find(tok);
                if( def != dict.end() )
                   {// Got a macro
                    if(skipsub)
                       {// Don't substitute, leave out the '#'
                        fout << tok;
                       }
                    else
                       {// Substitute
                        fout << def->second;
                        ++res.n_sub;
                       }
                   }
                else
                   {// Not a define, pass as it is
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
    return res;
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
