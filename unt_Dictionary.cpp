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
    std::ifstream fin( pth );
    if( !fin )
       {
        std::cerr << "!! Cannot open " << pth << std::endl;
        return 1;
       }

    // (2) Parse the file
    enum{ ST_NEWLINE, ST_SKIPLINE, ST_DEFINE, ST_DEF, ST_MACRO, ST_STRING } status = ST_NEWLINE;
    int issues = 0;
    std::string token;
    char c;
    while( fin.get(c) )
       {
        switch( status )
           {
            case ST_NEWLINE :
                // Skip spaces
                if( c=='\r' || c=='\n' || c=='\t' || c=='\ ' ) ;
                else if( c=='#' ) status = ST_DEFINE; // could be a '#define'
                else if( c=='D') status = ST_DEF; // could be a 'DEF'
                else status = ST_SKIPLINE; // Nothing interesting, comment or garbage, skip the line
                break;

            case ST_SKIPLINE :
                if(c=='\r' || c=='\n') status = ST_NEWLINE;
                break;

            case ST_DEFINE :
                     if(               c!='d') status = ST_SKIPLINE; // Garbage, skip the line
                else if(!fin.get(c) || c!='e') status = ST_SKIPLINE; // Garbage, skip the line
                else if(!fin.get(c) || c!='f') status = ST_SKIPLINE; // Garbage, skip the line
                else if(!fin.get(c) || c!='i') status = ST_SKIPLINE; // Garbage, skip the line
                else if(!fin.get(c) || c!='n') status = ST_SKIPLINE; // Garbage, skip the line
                else if(!fin.get(c) || c!='e') status = ST_SKIPLINE; // Garbage, skip the line
                else if(!fin.get(c) || c!='\ '
                                    || c!='\t') status = ST_SKIPLINE; // Garbage, skip the line
                else status = ST_MACRO;
                break;

            case ST_MACRO :
                while(fin.get(c) && c=='\ ' || c=='\t') // Skip spaces
                break;
           }
        c = fin.get()

            //auto ret = insert( value_type( rit->str(1), rit->str(2) ) );
            //if( !ret.second )
            //   {
            //    std::cerr << "\'" << ret.first->first << "\' already existed (was \'" << ret.first->second << "\') " << std::endl;
            //    ++issues;
            //   }
       }
    fin.close();

    // (3) Finally
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
