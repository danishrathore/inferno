#ifndef INFERNO_UTILITIES_SHAPE_WALKER_HXX
#define INFERNO_UTILITIES_SHAPE_WALKER_HXX


# include <boost/iterator/iterator_facade.hpp>

#include "inferno/utilities/small_vector.hxx"
#include "inferno/inferno.hxx"

namespace inferno{



/** \brief helper class to iterate over all
    configurations of a certain shape.

    Usually this class is
    used to build general iterators
    for discrete factors and discrete value tables.

    \warning Using this class to iterate over
    all configurations is way more expensive
    then nested for loops.
    But since these nested for loops cannot 
    be written down for any arity, we
    need this call as fall-back.
    In inferno, this class is mostly used
    for factors and value tables 
    with an arity larger then 5.

*/
template<class SHAPE_FUNCTOR, class OUT_ITER>
class ShapeWalker
{
public:
    ShapeWalker(
    )
    :   shape_(),
        dimension_(),
        outIter_(){ 

    }
    ShapeWalker(
        SHAPE_FUNCTOR shape,
        size_t dimension, 
        OUT_ITER outIter
    )
    :   shape_(shape),
        dimension_(dimension),
        outIter_(outIter){ 

    }

    ShapeWalker & operator++() {

        for(size_t d = 0; d < dimension_; ++d) {

            if(outIter_[d] +1 < shape_(d) ) {
                ++outIter_[d];
                INFERNO_ASSERT(outIter_[d]<shape_(d));
                break;
            }
            else {
                if(d != dimension_ - 1) {
                    outIter_[d] = 0;
                }
                else {
                    outIter_[d]++;
                    break;
                }
            }
            
        }
        return *this;
    };
private:
    SHAPE_FUNCTOR shape_;
    OUT_ITER outIter_;
    size_t dimension_;
};


/** \brief forward iterate over all
    configurations of a certain shape.

    Usually this class is
    used to build general iterators
    for discrete factors and discrete value tables.

    \warning Using this class to iterate over
    all configurations is way more expensive
    then nested for loops.
    But since these nested for loops cannot 
    be written down for any arity, we
    need this call as fall-back.
    In inferno, this class is mostly used
    for factors and value tables 
    with an arity larger then 5.

*/
template<class SHAPE_FUNCTOR>
class ConfIterator
  : public boost::iterator_facade<
        ConfIterator<SHAPE_FUNCTOR>,
        const SmallVector<DiscreteLabel> &,
        boost::forward_traversal_tag
    >
{
    typedef SmallVector<DiscreteLabel>::iterator SVIter;
 public:
    ConfIterator()
    :   shape_(SHAPE_FUNCTOR()),
        conf_(SmallVector<DiscreteLabel>()),
        confIndex_(0),
        nConf_(0){
    }
    ConfIterator(
        const size_t nConf
    )
    :   shape_(SHAPE_FUNCTOR()),
        conf_(SmallVector<DiscreteLabel>()),
        confIndex_(nConf),
        nConf_(nConf){
    }
    ConfIterator(
        const SHAPE_FUNCTOR & shape, 
        const size_t dim, 
        const size_t nConf
    )
    :   shape_(shape),
        conf_(dim,0),
        confIndex_(0),
        nConf_(nConf),
        walker_(){
    }

    ConfIterator getEnd()const{
        return ConfIterator(nConf_);
    }

 private:

    friend class boost::iterator_core_access;

    void increment() { 
        if(confIndex_==0)
            walker_ = ShapeWalker<SHAPE_FUNCTOR, SVIter >(shape_, conf_.size(), conf_.begin());
        ++walker_;
        ++confIndex_;
    }

    bool equal(ConfIterator const& other) const{
        return other.confIndex_ == confIndex_;
    }

    const SmallVector<DiscreteLabel> & dereference() const { 
        return conf_;
    }
    SHAPE_FUNCTOR               shape_;
    SmallVector<DiscreteLabel>  conf_;
    int64_t confIndex_;
    int64_t nConf_;
    ShapeWalker<SHAPE_FUNCTOR, SVIter> walker_;
};




} // end namespace inferno

#endif /* INFERNO_UTILITIES_SHAPE_WALKER_HXX */
