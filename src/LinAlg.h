//
// Created by zzz on 12/4/22.
//

#ifndef ASRLAB_LINALG_H
#define ASRLAB_LINALG_H

#include <utility>
#include <valarray>
#include <memory>
#include <array>
#include <vector>
#include <numeric>
#include <iostream>
#include <cassert>
#include <limits>
#include "Types.hpp"

namespace {
    bool operator==(const std::valarray<size_t>& a, const std::valarray<size_t>& b) {
        //valarray has already the operator==, why should we define it again?
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] != b[i])
                return false;
        }
        return true;
    }
}

namespace linalg {

    using BaseT = std::valarray<float>;

    class Matrix;
    class Vector;


    class LinObj {
    public:
        struct Sect {//? After this series of operations really did not understand
            Sect(size_t s, size_t e) : s(s), e(e) {}
            Sect(size_t i) : s(i), e(i+1) {}
            bool operator==(const Sect& o) const {return s == o.s && e == o.e; }

            size_t s;
            size_t e;
        };

        LinObj(std::shared_ptr<BaseT> data, std::valarray<size_t> shape, const size_t& start = 0) : data(std::move(data)) {
            slice_from_param(std::move(shape), start);
            //a small question, can slice_from_param be defined after its use here?
        }

        LinObj(const std::valarray<size_t>& shape) :
            LinObj(std::make_shared<BaseT>(std::accumulate(begin(shape), end(shape), 1, std::multiplies<size_t>{})),shape){}
            //std::accumulate() has only 3 parameters?

        LinObj(std::shared_ptr<BaseT> data, std::gslice const& slice) : data(std::move(data)), slice(slice) {}

        LinObj& operator=(const LinObj& other) {//赋值也要重写？
            if (&other != this) {
                assert(other.slice.size().size() == slice.size().size());
                for (size_t i = 0; i < slice.size().size(); i++)
                    assert(other.slice.size()[i] == slice.size()[i]);
                BaseT tmp = other.get();
                get() = tmp;
                //get()=other.get(); ?
            }
            return *this;
        }

        void reset(std::shared_ptr<BaseT> datap, std::valarray<size_t> shape, const size_t& start = 0) {
            data = std::move(datap);
            slice_from_param(std::move(shape), start);
        }
        void slice_from_param(std::valarray<size_t> shape, const size_t& start = 0) {
            size_t stride = 1;
            std::valarray<size_t> strides(shape.size());
            for (size_t i = shape.size(); i > 0; i--) {
                strides[i -1] = stride;
                stride *= shape[i -1];
            }
            slice = std::gslice(start, shape, strides);
        }

        [[nodiscard]] std::gslice_array<float> get() const { return (*data)[slice]; }
        [[nodiscard]] std::valarray<size_t> shape() const { return slice.size();}

        LinObj& operator*=(const LinObj& other) {
            assert(shape() == other.shape());
            get() *= other.get();
            return *this;
        }
        LinObj& operator+=(const LinObj& other) {
            assert(shape() == other.shape());
            get() += other.get();
            return *this;
        }
        void tanh();
        void relu();
        void sigmoid();

        void dtanh();
        void drelu();
        void dsigmoid();
        LinObj operator*(const LinObj& other) const {
            BaseT ret = get();
            ret *= BaseT(other.get());
            return {std::make_shared<BaseT>(std::move(ret)), shape()};
        }
        LinObj operator-(const LinObj& other) const {
            BaseT ret = get();
            ret -= BaseT(other.get());
            return {std::make_shared<BaseT>(std::move(ret)), shape()};
        }
        void nonzero() {
            BaseT tmp = get();
            for(auto& v : tmp)
                if (v==0)
                    v = 0.0000001;
            get() = tmp;
        }

        std::shared_ptr<BaseT> data;
        std::gslice slice;
    };

    static const LinObj::Sect ALL(0, std::numeric_limits<size_t>::infinity());

    class Vector : public LinObj {
    public:
        using LinObj::LinObj;
    //    Vector(std::shared_ptr<BaseT> data, const std::slice& slicep) : LinObj(data, std::gslice(slicep.start(), {slicep.size()}, {slicep.stride()})) {}
        [[nodiscard]] float dot(const Vector& v) const;
        Vector(const LinObj& other) : LinObj(other) {
            assert(slice.size().size() == 1);
        }
        operator Vect() {
            Vect v(shape()[0]);
            for (size_t i = 0; i < shape()[0]; i++)
                v(i) = at(i);
            return v;
        }
        void softmax() {
            BaseT v = get();
            v = std::exp(v);
            v /= v.sum();
            get() = v;
        }
        [[nodiscard]] const float& at(size_t i) const { return (*data)[slice.start() + i * slice.stride()[0]]; }
        [[nodiscard]] float& at(size_t i) { return (*data)[slice.start() + i * slice.stride()[0]]; }
        [[nodiscard]] Matrix outer(const Vector& v) const;
    };

    std::ostream& operator<<(std::ostream& out, const Vector& v);

    class Matrix : public LinObj {
    public:
        using LinObj::LinObj;
    //    Matrix(std::shared_ptr<BaseT> data, const std::array<size_t, 2>& dims, const size_t& start = 0) : LinObj(data) {}
        Matrix(const LinObj& other) : LinObj(other) {
            assert(slice.size().size() == 2);
        }
        Matrix& operator=(const Matr& m) {
            assert(shape()[0] == m.rows() && shape()[1] == m.cols());
            for (size_t i = 0; i < shape()[0]; i++)
                for (size_t j = 0; j < shape()[1]; j++)
                     at(i, j) = m(i, j);
            return *this;
        }

        void softmax() {
            for (size_t i = 0; i < shape()[0]; ++i) {
                Vector v = operator[](i);
                v.softmax();
            }
        }
        Matrix& radd(const Vector& v) {
            for (size_t i = 0; i < shape()[0]; ++i)
                operator[](i) += v;
            return *this;
        }
        operator Matr() {
            Matr m(shape()[0], shape()[1]);
            for (size_t i = 0; i < shape()[0]; i++)
                for (size_t j = 0; j < shape()[1]; j++)
                    m(i, j) = at(i, j);
            return m;
        }

        float& at(size_t i, size_t j);
        const float& at(size_t i, size_t j) const;
        Vector operator[](size_t j) const;

        [[nodiscard]] Vector col(size_t j) const;
        [[nodiscard]] Matrix dot(const Matrix& m) const {
            assert(slice.size()[1] == m.slice.size()[0]);
            Matrix ret({shape()[0], m.shape()[1]});
            for (size_t j = 0; j < m.shape()[1]; j++)
                for (size_t i = 0; i < shape()[0]; i++)
                    ret.at(i, j) = operator[](i).dot(m.col(j));
            return ret;
        }
        [[nodiscard]] Matrix transpose() const {
            return {data, {slice.start(), {slice.size()[1], slice.size()[0]}, {slice.stride()[1], slice.stride()[0]}}};
        }
        [[nodiscard]] Vector sumy() const {
            Vector ret({slice.size()[0]});
            for (size_t i = 0; i < slice.size()[1]; i++) {
                ret += col(i);
            }
            return ret;
        }
    };

    Matrix operator+(const Matrix& m, const Vector& v);

    class Tensor : public LinObj {
    public:
        using LinObj::LinObj;
        Tensor operator() (const Sect& a, const Sect& b, const Sect& c) const;
        Tensor operator() (const Sect& a, const Sect& b, const Sect& c);
        void resize(size_t x, size_t y, size_t z) {
            data->resize(x * y * z);
            slice = std::gslice(0, {x, y, z}, {z*y,z,1});
        }

        Matrix mat() const;

        [[nodiscard]] Vector sumx() const;
        [[nodiscard]] Vector at(size_t i, size_t j) const;
    };
    bool operator==(const Matrix& m1, const Matr& m2);
}

#endif //ASRLAB_LINALG_H
