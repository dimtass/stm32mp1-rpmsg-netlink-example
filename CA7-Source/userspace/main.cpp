#include "log.h"
#include "netlink_tester.h"

void print_help(char * prog);

int main(int argc, char **argv)
{

    FILELog::ReportingLevel() = linfo;
    L_(linfo) << "Application started";

    NetlinkTester tester;

    L_(linfo) << "---- Creating tests ----";
    tester.add_test(512);
    tester.add_test(1024);
    tester.add_test(2048);
    tester.add_test(4096);
    tester.add_test(8192);
    tester.add_test(16384);
    tester.add_test(32768);

    L_(linfo) << "---- Starting tests ----";
    size_t n_tests = tester.get_num_of_tests();

    struct NetlinkTester::test_item item;
    for (size_t i; i<n_tests; i++) {
        tester.run_test(i, item);
    }

    return(0);
}

void print_help(char * prog)
{
    printf("This program benchmarks the RPMSG driver on the STM32MP1 using netlink.\n\n", prog, prog);
}
