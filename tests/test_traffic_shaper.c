#include "traffic_shaper.h"
#include <assert.h>


void test_apply_shaping_rules() {
    assert(apply_shaping_rules("eth0", 1000) == 0);
    assert(apply_shaping_rules("eth0", -1) == -1);
}


int main() {
    test_apply_shaping_rules();
    return 0;
}