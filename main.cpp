#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector> // Used to collect input paths
//#include <exception>
//---------------------------------------------------------------------------
#include "unt_Dictionary.h" // 'cls_Dictionary'



//---------------------------------------------------------------------------
// Process a file: my parser to tokenize properly
int Process(std::string& pth, const cls_Dictionary& dict)
{
    // (0) See extension
    //std::string ext( pth.find_last_of(".")!=std::string::npos ? pth.substr(pth.find_last_of(".")+1) : "" );
    //if( ext=="def" )

    // (1) Open input file for read
    std::cout << std::endl << '<' << pth << '>' << std::endl;
    std::ifstream fin( pth, std::ios::binary );
    if( !fin )
       {
        std::cerr << "  Cannot read the file!" << std::endl;
        return 1;
       }

    // (2) Open output file for write
    std::string pth_out = pth + ".tr";
    std::cout << " >> " << pth_out << std::endl;
    std::ofstream fout( pth_out, std::ios::binary ); // Overwrite
    if( !fout )
       {
        std::cerr << "  Cannot write the output file!" << std::endl;
        return 1;
       }

    // (3) Parse the file
    enum{ ST_SKIPLINE, ST_SEEK, ST_COLLECT } status = ST_SEEK;
    int n_tok=0, n_sub=0; // Number of encountered tokens and substitutions
    int l = 1; // Current line number
    std::string tok; // Bag for current token
    bool skipsub; // Auxiliary to handle '#' for skipping substitution
    // TODO 2: should deal with encoding?
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
                else if( c=='+' || c=='-' || c=='*' || c=='=' ||
                         c=='(' || c==')' || c=='[' || c==']' ||
                         c=='{' || c=='}' || c=='<' || c=='>' ||
                         c=='.' || c==',' || c==':' ||
                         c=='!' || c=='&' || c=='|' || c=='^' ||
                         c=='\'' || c=='\"' || c=='\\' )
                     {// Skip operators
                      fout << c;
                      c = fin.get();
                     }
                else if( c=='/' )
                     {// Skip division/detect c++ comment line
                      fout << c;
                      if( fin.get()=='/' )
                         {
                          fout << '/';
                          c = fin.get();
                          status = ST_SKIPLINE;
                         }
                     }
                else if( c==';' )
                     {// Skip semicolon/detect Fagor comment line
                      fout << c;
                      c = fin.get();
                      status = ST_SKIPLINE;
                     }
                else {// Got a token
                      // Handle the 'no-substitution' character
                      if( c=='#' )
                           {
                            skipsub = true;
                            tok = "";
                           }
                      else {
                            skipsub = false;
                            tok = c; // tok=""; tok+=c;
                           }
                      c = fin.get(); // Next
                      status = ST_COLLECT;
                     }
                break;

            case ST_COLLECT : // Collect the rest of the token
                // The token ends with control chars or operators
                while( c!=EOF && c>' ' &&
                       c!='+' && c!='-' && c!='*' && c!='=' &&
                       c!='(' && c!=')' && // '[', ']' can be part of the token
                       c!='{' && c!='}' && c!='<' && c!='>' &&
                       c!='.' && c!=',' && c!=':' && c!=';' &&
                       c!='!' && c!='&' && c!='|' && c!='^' &&
                       c!='\'' && c!='\"' && c!='\\' && c!='/' )
                   {
                    tok += c;
                    c = fin.get(); // Next
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
    std::cout << "  Expanded " << n_sub << " macros checking a total of " << n_tok << " tokens in " << l << " lines" << std::endl;
    return 0;
} // 'Process'


//---------------------------------------------------------------------------
enum {RET_OK=0, RET_ARGERR=1, RET_IOERR=2, RET_DEFERR=2, RET_PRCERR=3 };
int main( int argc, const char* argv[] )
{
    // (0) Local objects
    cls_Dictionary dict;
    std::vector<std::string> in_files;

    // (1) Command line arguments
    std::cout << "Running in: " << argv[0] << std::endl;
    bool including = false;
    bool inv = false;
    for( int i=1; i<argc; ++i )
       {
        //std::cout << i << " " << argv[i] << std::endl;
        std::string arg( argv[i] );

        if( arg[0] == '-' )
             {// A command switch
              if( arg.length()!=2 )
                    {
                     std::cerr << "!! Wrong command switch " << arg << std::endl;
                     return RET_ARGERR;
                    }
              else if( arg[1]=='i' )
                   {// Next path is a definition file to include in the dictionary
                    including = true;
                   }
              else if( arg[1]=='x' )
                   {// Must invert the dictionary
                    inv = true;
                   }
              else {
                    std::cerr << "!! Unknown command switch " << arg << std::endl;
                    return RET_ARGERR;
                   }
             }
        else {// A file path
              if( including )
                   {
                    int issues = dict.LoadFile( arg );
                    including = false;
                    if( issues>0 )
                       {
                        std::cerr << "! " << issues << " issues including " << arg << std::endl;
                        return RET_DEFERR;
                       }
                   }
              else {
                    // TODO: corresponding output file name
                    in_files.push_back( arg );
                   }
             }
       }
    if(inv) dict.Invert(true); // Exclude numbers
    //dict.Peek();

    // (2) Process the input files
    int issues = 0;
    for( auto i=in_files.begin(); i!=in_files.end(); ++i )
       {
        issues += Process(*i, dict);
       } // All the input files
    if( issues>0 )
       {
        std::cerr << std::endl << issues << " issues processing files!" << std::endl;
        return RET_PRCERR;
       }

    // (3) Finally
    return RET_OK;
} // end 'main'
