
#include "doctest/doctest.h"
#include "ghs/msg.hpp"
#include "ghs-demo-msgutils.hpp"
#include "miniz.h"
#include <string>
#include <sstream>

TEST_CASE("miniz-understanding")
{
  std::string msg = "Helloooooo Miniz!!!!!!!!!!!!!";
  MESSAGE(msg);
  size_t len = msg.size()+1;
  
  unsigned char cbuf[256];
  mz_ulong sz = 256;

  auto retcode = compress(cbuf,&sz,(unsigned char*)msg.c_str(),len);
  CHECK_EQ(retcode, Z_OK);
  CHECK_LT(sz,256);
  CHECK_LT(sz,msg.size());

  unsigned char ubuf[256];
  mz_ulong sz_out = 256;

  retcode = uncompress(ubuf,&sz_out,cbuf,sz);
  CHECK_EQ(retcode,Z_OK);

  CHECK_GT(sz_out,sz);
  CHECK_EQ(sz_out,len);

  std::stringstream ss;
  ss<<(char*)&ubuf[0];
  MESSAGE(ss.str());
  CHECK(ss.str() == msg);

}

TEST_CASE("miniz-aint-magic")
{
  std::string msg = "Helloooooo Miniz!";
  MESSAGE(msg);
  size_t len = msg.size()+1;
  
  unsigned char cbuf[256];
  mz_ulong sz = 256;

  auto retcode = compress(cbuf,&sz,(unsigned char*)msg.c_str(),len);
  CHECK_EQ(retcode, Z_OK);
  CHECK_LT(sz,256);

  //SUPRISE:
  CHECK_GT(sz,msg.size());

  unsigned char ubuf[256];
  mz_ulong sz_out = 256;

  retcode = uncompress(ubuf,&sz_out,cbuf,sz);
  CHECK_EQ(retcode,Z_OK);

  //SUPRISE TOO:
  CHECK_LT(sz_out,sz);

  CHECK_EQ(sz_out,len);

  std::stringstream ss;
  ss<<(char*)&ubuf[0];
  MESSAGE(ss.str());
  CHECK(ss.str() == msg);

}
