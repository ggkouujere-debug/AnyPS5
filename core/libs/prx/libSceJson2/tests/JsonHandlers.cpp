#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

struct Value { void* node; };
struct String { void* text; };
struct Array { void* items; };
struct Object { void* items; };
struct Iterator { void* position; };
struct Pair {
    String key;
    std::uint64_t reserved;
    Value value;
};

using ElementAccessFailureHandler =
    void (APS5_VABI*)(int, const String*, const unsigned long*, const Value*, const Value*, void*);
using SpecialFloatHandler = Value (APS5_VABI*)(std::int32_t, double, const Value*, void*, bool*);
using TypeMismatchHandler = const Value& (APS5_VABI*)(std::int32_t, const Value*, const Value*, void*);

extern "C" {
int APS5_VABI _ZN3sce4Json5Value30setElementAccessFailureHandlerEPFviPKNS0_6StringEPKmPKS1_S8_PvES9_(
    ElementAccessFailureHandler, void*);
int APS5_VABI _ZN3sce4Json5Value22setSpecialFloatHandlerEPFKS1_NS0_12FunctionTypeEdPS2_PvPbES5_(SpecialFloatHandler,
                                                                                             void*);
int APS5_VABI _ZN3sce4Json5Value22setTypeMismatchHandlerEPFRKS1_NS0_9ValueTypeEPS2_S5_PvES6_(TypeMismatchHandler, void*);
bool APS5_VABI _ZNK3sce4Json5ValuecvbEv(const Value*);
void APS5_VABI _ZN3sce4Json5ValueC1Ev(Value*);
void APS5_VABI _ZN3sce4Json5ValueC1Eb(Value*, bool);
void APS5_VABI _ZN3sce4Json5ValueC1El(Value*, std::int64_t);
void APS5_VABI _ZN3sce4Json5ValueC1Em(Value*, std::uint64_t);
void APS5_VABI _ZN3sce4Json5ValueC1Ed(Value*, double);
void APS5_VABI _ZN3sce4Json5ValueC1EPKc(Value*, const char*);
void APS5_VABI _ZN3sce4Json5ValueC1ERKNS0_5ArrayE(Value*, const Array*);
void APS5_VABI _ZN3sce4Json5ValueC1ERKNS0_6ObjectE(Value*, const Object*);
void APS5_VABI _ZN3sce4Json5ValueD1Ev(Value*);
void APS5_VABI _ZN3sce4Json6StringC1EPKc(String*, const char*);
void APS5_VABI _ZN3sce4Json6StringD1Ev(String*);
void APS5_VABI _ZN3sce4Json5ArrayC1Ev(Array*);
void APS5_VABI _ZN3sce4Json5ArrayD1Ev(Array*);
void APS5_VABI _ZN3sce4Json5Array9push_backERKNS0_5ValueE(Array*, const Value*);
void APS5_VABI _ZN3sce4Json6ObjectC1Ev(Object*);
void APS5_VABI _ZN3sce4Json6ObjectD1Ev(Object*);
int APS5_VABI _ZN3sce4Json6Object6insertERKNS1_4PairE(Object*, const Pair*);
std::size_t APS5_VABI _ZNK3sce4Json6Object4sizeEv(const Object*);
void APS5_VABI _ZN3sce4Json6Object4PairC1ERKNS0_6StringERKNS0_5ValueE(Pair*, const String*, const Value*);
void APS5_VABI _ZN3sce4Json6Object4PairD1Ev(Pair*);
Iterator* APS5_VABI _ZNK3sce4Json6Object3endEv(Iterator*, const Object*);
Iterator* APS5_VABI _ZNK3sce4Json6Object4findERKNS0_6StringE(Iterator*, const Object*, const String*);
bool APS5_VABI _ZNK3sce4Json6Object8iteratorneERKS2_(const Iterator*, const Iterator*);
Pair* APS5_VABI _ZNK3sce4Json6Object8iteratordeEv(const Iterator*);
const std::int64_t* APS5_VABI _ZNK3sce4Json5Value10getIntegerEv(const Value*);
}

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

template <typename Call>
bool ThrowsRuntime(Call call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void OnElementAccessFailure(int, const String*, const unsigned long*, const Value*, const Value*, void*) {}
Value OnSpecialFloat(std::int32_t, double, const Value*, void*, bool*) { return Value{}; }
const Value& OnTypeMismatch(std::int32_t, const Value*, const Value*, void*) {
    static Value fallback{};
    return fallback;
}

struct BoolValue {
    Value value{};
    explicit BoolValue(bool boolean) { _ZN3sce4Json5ValueC1Eb(&value, boolean); }
    ~BoolValue() { _ZN3sce4Json5ValueD1Ev(&value); }
};

void CheckOperatorBool() {
    Value null{};
    _ZN3sce4Json5ValueC1Ev(&null);
    Require(_ZNK3sce4Json5ValuecvbEv(&null) == false, "null must convert to false");
    _ZN3sce4Json5ValueD1Ev(&null);

    Value missing{};
    Require(ThrowsRuntime([&] { _ZNK3sce4Json5ValuecvbEv(&missing); }), "unconstructed value must throw");

    BoolValue yes(true);
    BoolValue no(false);
    Require(_ZNK3sce4Json5ValuecvbEv(&yes.value) == true, "true must convert to true");
    Require(_ZNK3sce4Json5ValuecvbEv(&no.value) == false, "false must convert to false");

    Value integer{};
    _ZN3sce4Json5ValueC1El(&integer, 0);
    Require(_ZNK3sce4Json5ValuecvbEv(&integer) == false, "0 must convert to false");
    _ZN3sce4Json5ValueD1Ev(&integer);
    _ZN3sce4Json5ValueC1El(&integer, 42);
    Require(_ZNK3sce4Json5ValuecvbEv(&integer) == true, "42 must convert to true");
    _ZN3sce4Json5ValueD1Ev(&integer);
    _ZN3sce4Json5ValueC1El(&integer, -1);
    Require(_ZNK3sce4Json5ValuecvbEv(&integer) == true, "-1 must convert to true");
    _ZN3sce4Json5ValueD1Ev(&integer);

    Value uinteger{};
    _ZN3sce4Json5ValueC1Em(&uinteger, 0);
    Require(_ZNK3sce4Json5ValuecvbEv(&uinteger) == false, "0u must convert to false");
    _ZN3sce4Json5ValueD1Ev(&uinteger);
    _ZN3sce4Json5ValueC1Em(&uinteger, 7);
    Require(_ZNK3sce4Json5ValuecvbEv(&uinteger) == true, "7u must convert to true");
    _ZN3sce4Json5ValueD1Ev(&uinteger);

    Value real{};
    _ZN3sce4Json5ValueC1Ed(&real, 0.0);
    Require(_ZNK3sce4Json5ValuecvbEv(&real) == false, "0.0 must convert to false");
    _ZN3sce4Json5ValueD1Ev(&real);
    _ZN3sce4Json5ValueC1Ed(&real, 1.5);
    Require(_ZNK3sce4Json5ValuecvbEv(&real) == true, "1.5 must convert to true");
    _ZN3sce4Json5ValueD1Ev(&real);

    Value text{};
    _ZN3sce4Json5ValueC1EPKc(&text, "");
    Require(_ZNK3sce4Json5ValuecvbEv(&text) == false, "empty string must convert to false");
    _ZN3sce4Json5ValueD1Ev(&text);
    _ZN3sce4Json5ValueC1EPKc(&text, "x");
    Require(_ZNK3sce4Json5ValuecvbEv(&text) == true, "non-empty string must convert to true");
    _ZN3sce4Json5ValueD1Ev(&text);

    Array array{};
    _ZN3sce4Json5ArrayC1Ev(&array);
    Value element{};
    _ZN3sce4Json5ValueC1Ev(&element);
    Value emptyArrayValue{};
    _ZN3sce4Json5ValueC1ERKNS0_5ArrayE(&emptyArrayValue, &array);
    Require(_ZNK3sce4Json5ValuecvbEv(&emptyArrayValue) == false, "empty array must convert to false");
    _ZN3sce4Json5Array9push_backERKNS0_5ValueE(&array, &element);
    Value arrayValue{};
    _ZN3sce4Json5ValueC1ERKNS0_5ArrayE(&arrayValue, &array);
    Require(_ZNK3sce4Json5ValuecvbEv(&arrayValue) == true, "non-empty array must convert to true");
    _ZN3sce4Json5ValueD1Ev(&arrayValue);
    _ZN3sce4Json5ValueD1Ev(&emptyArrayValue);
    _ZN3sce4Json5ValueD1Ev(&element);
    _ZN3sce4Json5ArrayD1Ev(&array);

    Object object{};
    _ZN3sce4Json6ObjectC1Ev(&object);
    Value emptyObjectValue{};
    _ZN3sce4Json5ValueC1ERKNS0_6ObjectE(&emptyObjectValue, &object);
    Require(_ZNK3sce4Json5ValuecvbEv(&emptyObjectValue) == false, "empty object must convert to false");
    _ZN3sce4Json5ValueD1Ev(&emptyObjectValue);
    String key{};
    _ZN3sce4Json6StringC1EPKc(&key, "key");
    Value fortyTwo{};
    _ZN3sce4Json5ValueC1El(&fortyTwo, 42);
    Pair pair{};
    _ZN3sce4Json6Object4PairC1ERKNS0_6StringERKNS0_5ValueE(&pair, &key, &fortyTwo);
    Require(_ZN3sce4Json6Object6insertERKNS1_4PairE(&object, &pair) == 0, "insert must return 0");
    Value objectValue{};
    _ZN3sce4Json5ValueC1ERKNS0_6ObjectE(&objectValue, &object);
    Require(_ZNK3sce4Json5ValuecvbEv(&objectValue) == true, "non-empty object must convert to true");
    Require(_ZNK3sce4Json6Object4sizeEv(&object) == 1, "object must hold one pair after insert");
    Iterator found{};
    Iterator done{};
    _ZNK3sce4Json6Object4findERKNS0_6StringE(&found, &object, &key);
    _ZNK3sce4Json6Object3endEv(&done, &object);
    Require(_ZNK3sce4Json6Object8iteratorneERKS2_(&found, &done), "inserted key must be findable");
    Require(*_ZNK3sce4Json5Value10getIntegerEv(&_ZNK3sce4Json6Object8iteratordeEv(&found)->value) == 42,
            "inserted pair must keep its value");
    _ZN3sce4Json6Object4PairD1Ev(&pair);
    _ZN3sce4Json5ValueD1Ev(&fortyTwo);
    _ZN3sce4Json6StringD1Ev(&key);
    _ZN3sce4Json5ValueD1Ev(&objectValue);
    _ZN3sce4Json6ObjectD1Ev(&object);
}

void CheckHandlerSetters() {
    void* cookie = reinterpret_cast<void*>(0x1234);
    Require(_ZN3sce4Json5Value30setElementAccessFailureHandlerEPFviPKNS0_6StringEPKmPKS1_S8_PvES9_(
                &OnElementAccessFailure, cookie) == 0,
            "setElementAccessFailureHandler must return 0");
    Require(_ZN3sce4Json5Value22setSpecialFloatHandlerEPFKS1_NS0_12FunctionTypeEdPS2_PvPbES5_(&OnSpecialFloat, cookie) ==
                0,
            "setSpecialFloatHandler must return 0");
    Require(_ZN3sce4Json5Value22setTypeMismatchHandlerEPFRKS1_NS0_9ValueTypeEPS2_S5_PvES6_(&OnTypeMismatch, cookie) == 0,
            "setTypeMismatchHandler must return 0");
    Require(_ZN3sce4Json5Value30setElementAccessFailureHandlerEPFviPKNS0_6StringEPKmPKS1_S8_PvES9_(nullptr, nullptr) ==
                0,
            "clearing the element access failure handler must return 0");
    Require(_ZN3sce4Json5Value22setSpecialFloatHandlerEPFKS1_NS0_12FunctionTypeEdPS2_PvPbES5_(nullptr, nullptr) == 0,
            "clearing the special float handler must return 0");
    Require(_ZN3sce4Json5Value22setTypeMismatchHandlerEPFRKS1_NS0_9ValueTypeEPS2_S5_PvES6_(nullptr, nullptr) == 0,
            "clearing the type mismatch handler must return 0");
}

}

int main() {
    CheckOperatorBool();
    CheckHandlerSetters();
    std::puts("JSON handler tests passed");
    return 0;
}
