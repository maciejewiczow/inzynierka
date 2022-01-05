#ifndef TRIDIAGONAL_HEADER_GUARD
#define TRIDIAGONAL_HEADER_GUARD

#include <BasicLinearAlgebra.h>
/*
    Storage class for `BLA::Matrix` that stores only elements on the
    diagonal, superdiagonal and subdiagonal of the matrix. All others are reported as 0
*/
template<size_t dim, typename T = float>
class Tridiagonal {
private:
    T superDiagonal[dim-1];
    T diagonal[dim];
    T subDiagonal[dim-1];

    mutable T offDiagonal = 0;

public:
    using elem_t = T;

    const T& operator()(int row, int col) const {
        if (row == col)
            return diagonal[row];

        if (row+1 == col)
            return superDiagonal[row];

        if (row == col+1)
            return subDiagonal[col];

        return (offDiagonal = 0);
    }

    T& operator()(int row, int col) {
        return const_cast<T&>(static_cast<const Tridiagonal&>(*this).operator()(row, col));
    }
};

template<size_t dim, typename T = float>
using TridiagMat = BLA::Matrix<dim, dim, Tridiagonal<dim, T>>;

#endif
