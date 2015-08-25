#include "unt_Dictionary.h" // 'cls_Dictionary'
//#include <exception>
#include <cassert> // 'assert'
#include <iostream> // 'std::cerr'
#include <fstream> // 'std::ifstream'
#include <regex> // 'std::regex_iterator'


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
// A simple fast parser
int cls_Dictionary::LoadFile( const std::string& pth )
{
    // (0) See syntax (extension)
    //std::string ext( pth.find_last_of(".")!=std::string::npos ? pth.substr(pth.find_last_of(".")+1) : "" );
    //if( ext=="def" )

    // (1) Open file for read
    std::cout << std::endl << '[' << pth << ']' << std::endl;
    std::ifstream fin( pth );
    if( !fin )
       {
        std::cerr << "  Cannot read the file!" << std::endl;
        return 1;
       }

    // (2) Parse the file
    enum{ ST_SKIPLINE, ST_NEWLINE, ST_DEFINE, ST_DEF, ST_MACRO, ST_EXP, ST_ENDLINE } status = ST_NEWLINE;
    int issues = 0;
    std::string def,exp;
    int l = 1; // Current line number
    char c = fin.get();
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
                      auto ret = insert( value_type( def, exp ) );
                      if( !ret.second )
                         {
                          ++issues;
                          std::cerr << "  \'" << ret.first->first << "\' already existed in line " << l << " (was \'" << ret.first->second << "\') " << std::endl;
                         }
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

    // (3) Finally
    std::cout << "  Collected " << size() << " defines in " << l << " lines" << std::endl;
    return issues;
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
