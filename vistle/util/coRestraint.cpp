/**************************************************************************\ 
 **                                                                        **
 **                                                                        **
 ** Description: Interface classes for application modules to the COVISE   **
 **              software environment                                      **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                             (C)1997 RUS                                **
 **                Computer Center University of Stuttgart                 **
 **                            Allmandring 30                              **
 **                            70550 Stuttgart                             **
 ** Author:                                                                **
 ** Date:                                                                  **
\**************************************************************************/

#include "coRestraint.h"

#include <sstream>
#include <cstdio>
#include <cctype>

using namespace vistle;

//==========================================================================
//
//==========================================================================
coRestraint::coRestraint()
: all(false)
, changed(true)
, stringCurrent(false)
{
}

//==========================================================================
//
//==========================================================================
coRestraint::~coRestraint()
{
}

//==========================================================================
//
//==========================================================================
void coRestraint::add(Index mi, Index ma)
{
   stringCurrent = false;
   changed = true;
   all = false;
   min.push_back(mi);
   max.push_back(ma);
}


//==========================================================================
//
//==========================================================================
void coRestraint::add(Index val)
{
   stringCurrent = false;
   changed = true;
   all = false;
   min.push_back(val);
   max.push_back(val);
}


//==========================================================================
//
//==========================================================================
void coRestraint::add(const std::string &selection)
{
   stringCurrent = false;
   changed = true;

   if (selection == "all") {
      min.clear();
      max.clear();
      all = true;
      return;
   }

   all = false;

   const char *c=selection.c_str();
   while(*c && !isdigit(*c))
      ++c;

   while (*c) {
      int inc=0;
      Index dumMax, dumMin;
      Index numNumbers = sscanf(c,"%zd-%zd%n",&dumMin,&dumMax,&inc);
      if(numNumbers>0) {
         if(numNumbers==1) {
            dumMax=dumMin;
            if(inc == 0) {
               // inc is 0 at least on windows if only one number is read
               while(*c && isdigit(*c))
                  ++c;
            }
         }
         min.push_back(dumMin);
         max.push_back(dumMax);
      }
      c += inc;
      while(*c && !isdigit(*c))
         ++c;
   }
}


//==========================================================================
//
//==========================================================================
void coRestraint::clear()
{
   stringCurrent = false;
   changed = true;
   all = false;
   min.clear();
   max.clear();
}


//==========================================================================
//
//==========================================================================
Index coRestraint::lower() const
{
   Index i=0, low;
   if (!min.empty())
      low = min[0];
   else
      return -1;
   while (i<min.size())
   {
      if ( (low>=min[i]) )
      {
         low = min[i];
      }
      i++;
   }
   return low;
}


//==========================================================================
//
//==========================================================================
Index coRestraint::upper() const
{
   Index i=0, up;
   if (!max.empty())
      up = max[0];
   else
      return -1;
   while (i<max.size())
   {
      if ( (up<=max[i]) )
      {
         up = max[i];
      }
      ++i;
   }
   return up;
}


//==========================================================================
//
//==========================================================================
bool coRestraint::operator ()(Index val) const
{
   if (all)
      return true;

   Index i=0;
   while (i<min.size())
   {
      if ( (val>=min[i]) && (val<=max[i]) )
         return true;
      i++;
   }
   return false;
}


//==========================================================================
//
//==========================================================================
bool coRestraint::get(Index val, Index &group) const
{
   if (all) {
      group = -1;
      return true;
   }

   group=0;
   while (group<min.size())
   {
      if ( (val>=min[group]) && (val<=max[group]) )
         return true;
      group++;
   }
   return false;
}

//==========================================================================
//
//==========================================================================

const std::string &coRestraint::getRestraintString() const
{
   if (!stringCurrent)
   {
      stringCurrent = true;
      restraintString = getRestraintString(getValues());
   }
   return restraintString;
}

const std::string coRestraint::getRestraintString(std::vector<Index> sortedValues) const
{
   if (all)
      return "all";

   const Index size=sortedValues.size();
   if (size == 0)
      return "";

   std::ostringstream restraintStream;
   Index old=-1;
   bool started=false, firsttime=true;
   for(Index i=0; i<size; ++i)
   {
      Index actual = sortedValues[i];
      if (firsttime)
      {
         firsttime = false;
         restraintStream << actual;
         old=actual;
         continue;
      }
      else if ( actual == old+1 && i<size-1)
      {
         if (!started)
         {
            restraintStream << "-";
            started = true;
         }
         old = actual;
         continue;
      }
      else if (started)
      {
         started = false;
         restraintStream << old << ", " << actual;
      }
      else 
      {
         restraintStream << ", " << actual;
      }
      old = actual;
   }
   return restraintStream.str();
}

//==========================================================================
//
//==========================================================================
// function returns vector containing all integer indices
// that are specified by the string added to this coRestraint object
//
// returns an empty vector, if the evaluation of char array is 
// not successful
// 
const std::vector<Index> &coRestraint::getValues() const
{
   if (changed)
   {
      changed = false;
      values.clear();
      //getting the indices
      Index counter = lower();
      Index limit = upper();
      if (limit == -1 || counter == -1)
      {
         values.clear();
      }
      else
      {
         while( counter <= limit )
         {
            if( operator()(counter) )
            {
               values.push_back(counter);
            }
            ++counter;
         }
      }
   }

   return values;
}

