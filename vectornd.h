#pragma once

#include <vector>
#include <array>
#include <cassert>

template<class T, int D>
class vectornd : private std::vector<T>
{
public:
    static const int dimension = D;
    using std::vector<T>::reference;
    using std::vector<T>::const_reference;
    typedef std::array<std::size_t, D> size_type;
    using std::vector<T>::value_type;

    vectornd() {}
    // TODO Move constructor
    vectornd(const size_type& size)
        : std::vector<T>(flatten(size))
        , m_size(size)
    {}

    vectornd(const size_type& size, const value_type& value)
        : std::vector<T>(flatten(size), value)
        , m_size(size)
    {}

    const size_type& size() const { return m_size; }

    // TODO resize; // TODO Keep the shape when reszing ???

    // TODO What to do with functions that change the size
    // ie insert, push_back, erase

    reference at(const size_type& pos) { return std::vector<T>::at(index(pos)); }
    const_reference at(const size_type& pos) const { return std::vector<T>::at(index(pos)); }

    reference operator[](const size_type& pos) { return at(pos); }
    const_reference operator[](const size_type& pos) const { return at(pos); }

    bool operator==(const vectornd& other) const { return tie() == tie(); }
    bool operator<(const vectornd& other) const { return tie() < tie(); }

    void swap(vectornd& other) const { vector().swap(other.vector()); m_size.swap(other.m_size); }

//private:
    std::size_t index(const size_type& pos) const
    {
        std::size_t i = 0;
        for (int d = 0; d < dimension; ++d)
        {
            assert(pos[d] >= 0);
            assert(pos[d] < m_size[d]);
            i = i * m_size[d] + pos[d];
        }
        return i;
    }

private:
    static std::size_t flatten(const size_type& size)
    {
        std::size_t i = 1;
        for (auto r : size)
            i *= r;
        return i;
    }

    auto tie() const { return tie(vector(), m_size); }
    std::vector<T>& vector() { return this; }
    const std::vector<T>& vector() const { return this; }

    size_type m_size;
};

inline void TestVectorND()
{
    vectornd<int, 3> a3({ 3, 4, 5 });
    assert(a3.index({ 0, 0, 0 }) == 0);
    assert(a3.index({ 0, 0, 1 }) == 1); // TODO Indeces in this order or reverse order?
    assert(a3.index({ 0, 1, 0 }) == 5);
    assert(a3.index({ 1, 0, 0 }) == 4 * 5);

    a3.at({ 1, 0, 0 }) = 9;
    a3[{ 1, 0, 0 }] = 9;
}
