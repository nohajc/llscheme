#include <vector>
#include "../../include/common.hpp"
#include "../../include/any_ptr.hpp"
#include "../../include/parser.hpp"
#include <UnitTest++/UnitTest++.h>

using namespace llscm;

TEST(AnyPtrTest) {
    {
        any_ptr str = "Nice.";
        CHECK(str.type() == typeid(char));
        CHECK(APC<char>(str) != nullptr);

        any_ptr i = new int(1);
        CHECK(i.type() == typeid(int));
        CHECK(APC<int>(i) != nullptr);

        any_ptr d = new double(3.14159);
        CHECK(d.type() == typeid(double));
        CHECK(APC<double>(d) != nullptr);

        any_ptr str2;
        str2 = "It works!";
        CHECK(str2.type() == typeid(char));
        CHECK(APC<char>(str2) != nullptr);

        vector<any_ptr> vec = { str, i, d, str2 };
        CHECK_EQUAL(vec.size(), 4);

        any_ptr ap;
        ap = "string";
        CHECK(APC<char>(ap) != nullptr);
        ap = new StringReader("ahoj");
        CHECK(APC<StringReader>(ap) != nullptr);
        delete APC<StringReader>(ap);


        delete APC<int>(i);
        delete APC<double>(d);
    }
}
