/** \file qpbo.hxx 
    \brief  Implementation of inferno::inference::Qpbo  

    \warning To include this header one needs qpbo
    from  inferno_externals
    (in CMake ''WITH_QPBO'')
*/
#ifndef INFERNO_INFERENCE_QPBO_HXX
#define INFERNO_INFERENCE_QPBO_HXX

#include <unordered_map>

#include "inferno/inference/base_discrete_inference.hxx"
#include "inferno/inference/utilities/fix-fusion/higher-order-energy.hpp"
#include "inferno/inference/utilities/fix-fusion/clique.hpp"

#include <inferno_externals/qpbo/QPBO.h>


namespace inferno{
namespace inference{


    ///Option object for Qpbo
    struct QpboOptions : public InferenceOptions
    {
        /** \brief Different Qpbo algorithm 
            which can be used within Qpbo

            \warning If an other value than QpboAlgorithm::Automatic is chosen,
            it is the users responsibility that the choice is matching 
            the model.
            For example calling simple qpbo for a model with 
            a factor order greater then two will lead to 
            undefined behavior.
        */
        enum QpboAlgorithm{
            /// Automatically select the most suitable Qpbo algorithm
            Automatic  = 0,  
            /// Simple Qpbo only for second order models with binary labels    
            SimpleQpbo = 1,      
            /// Higher order Qpbo for models with order up to 9 and binary labels
            HigherOrderQpbo = 2,
            /// Multilabel Qpbo  for second order models with binary labels
            MultiLabelQpbo = 3  
        };

        /**  \brief Which qpbo algorithm should be used
        */
        QpboAlgorithm qpboAlgorithm_;
        /// using probeing technique
        bool useProbeing;
        /// \brief forcing strong persistency
        /// so far it is NOT recommended to
        /// enable this option
        bool strongPersistency;
        /// using improving technique
        bool useImproveing;

        /// \brief constructor
        QpboOptions() {
            qpboAlgorithm_ = Automatic; 
            strongPersistency = true;
            useImproveing = false;
            useProbeing = false;
        }
    };

    /** \brief Qpbo algorithm
        Qpbo for second order graphical models with binary
        variables.

        \ingroup discrete_inference partial_optimal_discrete_inference approximate_discrete_inference
    */
    template<class MODEL>
    class Qpbo : public DiscreteInferenceBase<Qpbo<MODEL>, MODEL > {
    private:
        struct ModelInfo{
            size_t maxArity;
            DiscreteLabel minNumLabels;
            DiscreteLabel maxNumLabels;

        };
    public:
        typedef kolmogorov::qpbo::QPBO<ValueType> QpboSolver;
        typedef MODEL Model;
        typedef QpboOptions Options;

        Qpbo(const Model & model, const Options & options)
        :   model_(&model),
            options_(options),
            qpbo_(NULL),
            constTerm_(0.0),
            bound_(-1.0*std::numeric_limits<ValueType>::infinity()),
            value_( 1.0*std::numeric_limits<ValueType>::infinity()),
            hoe_()
        {

            const auto maxArity = model_->maxArity();
            if(maxArity<=2){
                if(model_->denseVariableIds()){
                    const auto minVarId = model_->minVarId();
                    const auto nVar = model_->nVariables();             
                    qpbo_ = new QpboSolver(nVar, model_->nPairwiseFactors());
                    qpbo_->AddNode(nVar);

                    for(const auto factor : model_->factors()){
                        const auto arity = factor->arity();
                        if(arity == 0){
                            constTerm_ += factor->eval(0l);
                        }
                        else if(arity == 1){
                            const int qpboVi0 = factor->vi(0)-minVarId;
                            qpbo_->AddUnaryTerm(qpboVi0, factor->eval1(0l), factor->eval1(1l));
                        }
                        else if(arity == 2){
                            const int qpboVi0 = factor->vi(0)-minVarId;
                            const int qpboVi1 = factor->vi(1)-minVarId;

                            qpbo_->AddPairwiseTerm(qpboVi0, qpboVi1,
                                                   factor->eval2(0,0), factor->eval2(0,1),
                                                   factor->eval2(1,0), factor->eval2(1,1));
                        }
                        else
                            throw RuntimeError("INTERNAL ERROR: model_.maxArity() must have a bug");
                    }
                    qpbo_->MergeParallelEdges();
                }
                else{
                    throw NotImplementedException("models with non-dense variable ids are not yet supported");
                }
            }
            else{
                if(model_->denseVariableIds()){
                    
                }
                else{
                    throw NotImplementedException("models with non-dense variable ids are not yet supported");
                }
            }
            
        }


        

        ~Qpbo(){
            delete qpbo_;
        }



        void infer(){
            
            qpbo_->Solve();
            if(!options_.strongPersistency)
                qpbo_->ComputeWeakPersistencies();

            bound_ = constTerm_ + 0.5 * qpbo_->ComputeTwiceLowerBound();

            if(options_.useProbeing){
                throw NotImplementedException("useProbeing will be implemented soon");
            }
            if(options_.useImproveing){
                throw NotImplementedException("useImproveing will be implemented soon");
            }

        }

        template<class CONF_MAP>
        void conf(CONF_MAP & configuration){
            if(model_->denseVariableIds()){
                const auto minVarId = model_->minVarId();
                for(auto vi : model_->variableIds()){
                    configuration[vi-minVarId] = qpbo_->GetLabel(vi-minVarId);
                }
            }
        }

    private:
        


    private:
        const Model * model_;
        QpboOptions   options_;
        QpboSolver *  qpbo_;
        ValueType     constTerm_;
        ValueType     bound_;
        ValueType     value_;
        HigherOrderEnergy<ValueType, 10> hoe_;

    };

} // end namespace inference
} // end namespace inferno

#endif /* INFERNO_INFERENCE_QPBO_HXX */
