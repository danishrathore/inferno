#ifndef INFERNO_INFERENCE_QPBO_HXX
#define INFERNO_INFERENCE_QPBO_HXX

#include "inferno/inference/base_discrete_inference.hxx"
#include <inferno_externals/qpbo/QPBO.h>

namespace inferno{
namespace inference{


    ///Option object for Qpbo
    struct QpboOptions : public InferenceOptions
    {
        /// using probeing technique
        bool useProbeing;
        /// forcing strong persistency
        bool strongPersistency;
        /// using improving technique
        bool useImproveing;

        /// \brief constructor
        QpboOptions() {
           strongPersistency = true;
           useImproveing = false;
           useProbeing = false;
        }
    };

    template<class MODEL>
    class Qpbo : public DiscreteInferenceBase<Qpbo<MODEL>, MODEL > {

    public:
        typedef kolmogorov::qpbo::QPBO<ValueType> QpboSolver;
        typedef QpboOptions Options;
        typedef MODEL Model;
        Qpbo(const Model & model, const Options & options)
        :   model_(&model),
            options_(options),
            qpbo_(NULL),
            constTerm_(0.0),
            bound_(-1.0*std::numeric_limits<ValueType>::infinity()),
            value_( 1.0*std::numeric_limits<ValueType>::infinity())
        {
            if(options_.semiRing != MinSum)
                throw NotImplementedException("only MinSum is currently implemented");

            if(model_->denseVariableIds()){
                const auto minVarId = model_->minVarId();
                const auto nVar = model_->nVariables();
                INFERNO_CHECK_OP(model_->maxArity(),<=,2, "qpbo allows only models with a maximal arity of 2");
                qpbo_ = new QpboSolver(nVar, model_->nPairwiseFactors());
                qpbo_->AddNode(nVar);

                for(const auto factor : model_->factors()){
                    const auto arity = factor->arity();
                    if(arity == 0){
                        constTerm_ += factor->eval(0l);
                    }
                    else if(arity == 1){
                        const int qpboVi0 = factor->vi(0)-minVarId;
                        qpbo_->AddUnaryTerm(qpboVi0, factor->eval(0l), factor->eval(1l));
                    }
                    else if(arity == 2){
                        const int qpboVi0 = factor->vi(0)-minVarId;
                        const int qpboVi1 = factor->vi(1)-minVarId;

                        qpbo_->AddPairwiseTerm(qpboVi0, qpboVi1,
                                               factor->eval(0,0), factor->eval(0,1),
                                               factor->eval(1,0), factor->eval(1,1));
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

        template<class OUT_ITER>
        void conf(OUT_ITER configuration){
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
    };

} // end namespace inference
} // end namespace inferno

#endif /* INFERNO_INFERENCE_QPBO_HXX */