#include "parser.h"
#include "parser/imath.h"
#include "parser/type.h"
#include <math.h>
#include <stdlib.h>


bool numberFitsInIntIMath(u64 bits, mpz_t a, bool isNeg) {
    mpz_t s;
    mp_int_init(&s);
    mp_int_set_value(&s, 1);
    while (bits > 2) {
        mp_int_mul_pow2(&s, 2, &s);
        bits--;
    }
    if (isNeg) {
        int cmp = mp_int_compare(&a, &s);
        mp_int_clear(&s);
        if (cmp < 0) {
            return false;
        }
        return true;
    } else {
        int cmp = mp_int_compare(&a, &s);
        mp_int_clear(&s);
        if (cmp >= 0) {
            return false;
        }
        return true;
    }
}

bool numberFitsInInt(u64 bits, char* number) {
    mpz_t a;
    mp_int_init(&a);
    bool isNeg = number[0] == '-';
    mp_int_read_string(&a, 10, number);
    bool result = numberFitsInIntIMath(bits, a, isNeg);
    mp_int_clear(&a);
    return result;
}


bool numberFitsInUIntIMath(u64 bits, mpz_t a) {
    mpz_t s;
    mp_int_init(&s);
    mp_int_set_value(&s, 1);
    while (bits > 1) {
        mp_int_mul_pow2(&s, 2, &s);
        bits--;
    }

    int cmp = mp_int_compare(&a, &s);
    mp_int_clear(&s);
    if (cmp > 0) {
        return false;
    }
    return true;
}

bool numberFitsInUInt(u64 bits, char* number) {
    bool isNeg = number[0] == '-';
    if (isNeg) {
        return false;
    }
    mpz_t a;
    mp_int_init(&a);
    mp_int_read_string(&a, 10, number);
    bool result = numberFitsInUIntIMath(bits, a);
    mp_int_clear(&a);
    return result;
}

bool numberIsCompatalbeWithInt(char* number) {
    char* c = number;
    bool isFloat = true;
    while (*c != '\0') {
        if (*c == '.') {
            isFloat = true;
            break;
        }
        c++;
    }
    if (isFloat) {
        while (*c != '\0') {
            if (*c != '0') {
                return false;
            }
            c++;
        }
    }
    return true;
}

mpz_t getBigIntForConstNumber(Expression* expr, Arena* arena, bool* err) {
    *err = false;
    assert(getTypeFromId(expr->tid)->kind == tk_const_number);
    if (expr->type == ek_number) {
        mpz_t a;
        mp_int_init(&a);
        if (numberIsCompatalbeWithInt(expr->number)) {
            mp_int_read_string(&a, 10, expr->number);
            return a;
        } else {
            mp_int_set_value(&a, 0);
            *err = true;
            return a;
        }
    } else if (expr->type == ek_biop) {
        mpz_t left = getBigIntForConstNumber(expr->biopData->left, arena, err);
        if (*err) {
            return left;
        }
        mpz_t right = getBigIntForConstNumber(expr->biopData->right, arena, err);
        if (*err) {
            mp_int_clear(&right);
            return left;
        }
        switch (expr->biopData->operator) {
            case tt_add:
                mp_int_add(&left, &right, &left);
                mp_int_clear(&right);
                return left;
            case tt_sub:
                mp_int_sub(&left, &right, &left);
                mp_int_clear(&right);
                return left;
            case tt_mul:
                mp_int_mul(&left, &right, &left);
                mp_int_clear(&right);
                return left;
            case tt_div:
                mp_int_div(&left, &right, &left, &right);
                mp_int_clear(&right);
                return left;
            case tt_mod:
                mp_int_mod(&left, &right, &left);
                mp_int_clear(&right);
                return left;
            case tt_leq: {
                int cmp = mp_int_compare(&left, &right);
                if (cmp <= 0) {
                    mp_int_set_value(&left, 1);
                } else {
                    mp_int_set_value(&left, 0);
                }
                mp_int_clear(&right);
                return left;
            }
            case tt_geq: {
                int cmp = mp_int_compare(&left, &right);
                if (cmp >= 0) {
                    mp_int_set_value(&left, 1);
                } else {
                    mp_int_set_value(&left, 0);
                }
                mp_int_clear(&right);
                return left;
            }
            case tt_lt: {
                int cmp = mp_int_compare(&left, &right);
                if (cmp < 0) {
                    mp_int_set_value(&left, 1);
                } else {
                    mp_int_set_value(&left, 0);
                }
                mp_int_clear(&right);
                return left;
            }
            case tt_gt: {
                int cmp = mp_int_compare(&left, &right);
                if (cmp > 0) {
                    mp_int_set_value(&left, 1);
                } else {
                    mp_int_set_value(&left, 0);
                }
                mp_int_clear(&right);
                return left;
            }
            case tt_eq: {
                int cmp = mp_int_compare(&left, &right);
                if (cmp == 0) {
                    mp_int_set_value(&left, 1);
                } else {
                    mp_int_set_value(&left, 0);
                }
                mp_int_clear(&right);
                return left;
            }
            case tt_neq: {
                int cmp = mp_int_compare(&left, &right);
                if (cmp != 0) {
                    mp_int_set_value(&left, 1);
                } else {
                    mp_int_set_value(&left, 0);
                }
                mp_int_clear(&right);
                return left;
            }
            default: {
                *err = true;
                return left;
            }
        }
    } else {
        // things like sizeof i don't care to figure out this so we just put zero as 99.999% we catch overflows
        // and  sizeof can be target dependent and that is cancer to deal with
        mpz_t a;
        mp_int_init(&a);
        mp_int_set_value(&a, 0);
        return a;
    }
}

double getFloatForConstNumber(Expression* expr, Arena* arena, bool* err) {
    *err = false;
    assert(getTypeFromId(expr->tid)->kind == tk_const_number);

    if (expr->type == ek_number) {
        double value = strtod(expr->number, NULL);
        return value;
    } else if (expr->type == ek_biop) {
        double left = getFloatForConstNumber(expr->biopData->left, arena, err);
        if (*err) {
            return 0;
        }
        double right = getFloatForConstNumber(expr->biopData->right, arena, err);
        if (*err) {
            return 0;
        }
        switch (expr->biopData->operator) {
            case tt_add:
                return left + right;
            case tt_sub:
                return left - right;
            case tt_mul:
                return left * right;
            case tt_div:
                return left / right;
            case tt_mod:
                return fmod(left, right);
            case tt_leq:
                return left <= right;
            case tt_geq:
                return left >= right;
            case tt_lt:
                return left < right;
            case tt_gt:
                return left > right;
            case tt_eq:
                return left == right;
            case tt_neq:
                return left != right;
            default:
                *err = true;
                return 0;
        }
    } else {
        return 0;
    }
}

bool constExpressionNumberWorksWithType(Expression* expr, typeId tid, Arena* arena) {
    Type* type = getTypeFromId(tid);
    assert(getTypeFromId(expr->tid)->kind == tk_const_number);
    assert(type->kind == tk_int || type->kind == tk_uint || type->kind == tk_float);
    u64 bits = type->numberSize;
    if (type->kind == tk_float) {
        return true;
    }
    if (type->kind == tk_int) {
        bool err;
        mpz_t a = getBigIntForConstNumber(expr, arena, &err);
        int cmp = mp_int_compare_value(&a, 0);
        if (err) {
            mp_int_clear(&a);
            return false;
        }
        bool isNeg = cmp < 0;
        bool result = numberFitsInIntIMath(bits, a, isNeg);
        mp_int_clear(&a);
        return result;
    }
    if (type->kind == tk_uint) {
        bool err;
        mpz_t a = getBigIntForConstNumber(expr, arena, &err);
        if (err) {
            mp_int_clear(&a);
            return false;
        }
        int cmp = mp_int_compare_value(&a, 0);
        if (cmp < 0) {
            mp_int_clear(&a);
            return false;
        }
        bool result = numberFitsInUIntIMath(bits, a);
        mp_int_clear(&a);
        return result;
    }
    assert(false);
    return false;
}
