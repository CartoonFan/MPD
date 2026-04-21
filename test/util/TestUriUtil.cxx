/*
 * Unit tests for src/util/
 */

#include "util/UriUtil.hxx"

#include <gtest/gtest.h>

using std::string_view_literals::operator""sv;

TEST(UriUtil, RemoveAuth)
{
	EXPECT_EQ(uri_remove_auth("http://www.example.com/"),
		  ""sv);
	EXPECT_EQ(uri_remove_auth("http://foo:bar@www.example.com/"),
		  "http://www.example.com/"sv);
	EXPECT_EQ(uri_remove_auth("http://foo@www.example.com/"),
		  "http://www.example.com/"sv);
	EXPECT_EQ(uri_remove_auth("http://www.example.com/f:oo@bar"),
		  ""sv);
	EXPECT_EQ(uri_remove_auth("ftp://foo:bar@ftp.example.com/"),
		  "ftp://ftp.example.com/"sv);
}
