#include "CaseParserDriver.h"
#include "CaseLexer.h"
#include "CaseFile.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

void ensight::parser::error(const ensight::location &location, const std::string &msg)
{
    std::stringstream str;
    str << "parse error at " << location << ": " << msg;
    driver.lastError_ = str.str();
    std::cerr << driver.lastError_ << std::endl;
}

CaseParserDriver::CaseParserDriver(const std::string &sFileName)
{
    isOpen_ = false;

    location.initialize(&sFileName);

    inputFile_ = new std::ifstream(sFileName.c_str());
    if (!inputFile_->is_open()) {
        delete inputFile_;
        inputFile_ = nullptr;
        std::stringstream str;
        str << "could not open " << sFileName << " for reading";
        lastError_ = str.str();
        std::cerr << lastError_ << std::endl;
        return;
    }

    inputFile_->peek(); // try to exclude directories
    if (inputFile_->fail()) {
        delete inputFile_;
        inputFile_ = nullptr;
        std::stringstream str;
        str << "could not open " << sFileName << " for reading - fail";
        lastError_ = str.str();
        std::cerr << lastError_ << std::endl;
        return;
    }

    isOpen_ = true;

    lexer_ = new CaseLexer(inputFile_);
    lexer_->set_debug(1);
    //lexer_->set_debug( 0 );
}

CaseParserDriver::~CaseParserDriver()
{
    delete lexer_;
    lexer_ = nullptr;

    delete inputFile_;
    inputFile_ = nullptr;
}

std::string CaseParserDriver::lastError() const
{
    return lastError_;
}

void CaseParserDriver::setVerbose(bool enable)
{
    verbose_ = enable;
}

bool CaseParserDriver::parse()
{
    lexer_->set_debug(verbose_ ? 1 : 0);
    ensight::parser parse(*this);
    parse.set_debug_level(verbose_);
    return parse() == 0;
}

int ensightlex(ensight::parser::value_type *yylval, ensight::location *, CaseParserDriver &driver)
{
    return (driver.lexer_->scan(yylval));
}

CaseFile CaseParserDriver::getCaseObj()
{
    return caseFile_;
}

bool CaseParserDriver::isOpen()
{
    return isOpen_;
}
