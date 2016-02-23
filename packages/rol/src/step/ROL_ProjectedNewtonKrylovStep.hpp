// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef ROL_PROJECTEDNEWTONKRYLOVSTEP_H
#define ROL_PROJECTEDNEWTONKRYLOVSTEP_H

#include "ROL_Types.hpp"
#include "ROL_Step.hpp"

#include "ROL_Secant.hpp"
#include "ROL_Krylov.hpp"
#include "ROL_LinearOperator.hpp"

#include <sstream>
#include <iomanip>

/** @ingroup step_group
    \class ROL::ProjectedNewtonKrylovStep
    \brief Provides the interface to compute optimization steps
           with projected inexact ProjectedNewton's method using line search.
*/


namespace ROL {

template <class Real>
class ProjectedNewtonKrylovStep : public Step<Real> {
private:

  Teuchos::RCP<Secant<Real> >     secant_; ///< Secant object (used for quasi-Newton)
  Teuchos::RCP<Krylov<Real> >     krylov_; ///< Krylov solver object (used for inexact Newton)

  EKrylov ekv_;
  ESecant esec_;

  Teuchos::RCP<Vector<Real> > gp_;
  Teuchos::RCP<Vector<Real> > d_;

  Teuchos::RCP<LinearOperator<Real> > hessian_;
  Teuchos::RCP<LinearOperator<Real> > precond_;
 
  int iterKrylov_; ///< Number of Krylov iterations (used for inexact Newton)
  int flagKrylov_; ///< Termination flag for Krylov method (used for inexact Newton)
  int verbosity_;  ///< Verbosity level
 
  bool useSecantPrecond_; ///< Whether or not a secant approximation is used for preconditioning inexact Newton
  bool useProjectedGrad_; ///< Whether or not to use to the projected gradient criticality measure

  class HessianPNK : public LinearOperator<Real> {
  private:
    const Teuchos::RCP<Objective<Real> > obj_;
    const Teuchos::RCP<BoundConstraint<Real> > bnd_;
    const Teuchos::RCP<Vector<Real> > x_;
    const Teuchos::RCP<Vector<Real> > g_;
    Teuchos::RCP<Vector<Real> > v_;
    Real eps_;
  public:
    HessianPNK(const Teuchos::RCP<Objective<Real> > &obj,
               const Teuchos::RCP<BoundConstraint<Real> > &bnd,
               const Teuchos::RCP<Vector<Real> > &x,
               const Teuchos::RCP<Vector<Real> > &g,
               Real eps = 0.0 )
      : obj_(obj), bnd_(bnd), x_(x), g_(g), eps_(eps) {
      v_ = x_->clone();
    }
    void apply(Vector<Real> &Hv, const Vector<Real> &v, Real &tol) const {
      v_->set(v);
      bnd_->pruneActive(*v_,*g_,*x_,eps_);
      obj_->hessVec(Hv,*v_,*x_,tol);
      bnd_->pruneActive(Hv,*g_,*x_,eps_);
      v_->set(v);
      bnd_->pruneInactive(*v_,*g_,*x_,eps_);
      Hv.plus(v_->dual());
    }
  };

  class PrecondPNK : public LinearOperator<Real> {
  private:
    const Teuchos::RCP<Objective<Real> > obj_;
    const Teuchos::RCP<Secant<Real> > secant_;
    const Teuchos::RCP<BoundConstraint<Real> > bnd_;
    const Teuchos::RCP<Vector<Real> > x_;
    const Teuchos::RCP<Vector<Real> > g_;
    Teuchos::RCP<Vector<Real> > v_;
    Real eps_;
    const bool useSecant_;
  public:
    PrecondPNK(const Teuchos::RCP<Objective<Real> > &obj,
               const Teuchos::RCP<BoundConstraint<Real> > &bnd,
               const Teuchos::RCP<Vector<Real> > &x,
               const Teuchos::RCP<Vector<Real> > &g,
               Real eps = 0.0 )
      : obj_(obj), bnd_(bnd), x_(x), g_(g), eps_(eps), useSecant_(false) {
      v_ = x_->clone();
    }
    PrecondPNK(const Teuchos::RCP<Secant<Real> > &secant,
               const Teuchos::RCP<BoundConstraint<Real> > &bnd,
               const Teuchos::RCP<Vector<Real> > &x,
               const Teuchos::RCP<Vector<Real> > &g,
               Real eps = 0.0 )
      : secant_(secant), bnd_(bnd), x_(x), g_(g), eps_(eps), useSecant_(true) {
      v_ = x_->clone();
    }
    void apply(Vector<Real> &Hv, const Vector<Real> &v, Real &tol) const {
      Hv.set(v.dual()); 
    }
    void applyInverse(Vector<Real> &Hv, const Vector<Real> &v, Real &tol) const {
      v_->set(v);
      bnd_->pruneActive(*v_,*g_,*x_,eps_);
      if ( useSecant_ ) {
        secant_->applyH(Hv,*v_);
      }
      else {
        obj_->precond(Hv,*v_,*x_,tol);
      }
      bnd_->pruneActive(Hv,*g_,*x_,eps_);
      v_->set(v);
      bnd_->pruneInactive(*v_,*g_,*x_,eps_);
      Hv.plus(v_->dual());
    }
  };

public:

  using Step<Real>::initialize;
  using Step<Real>::compute;
  using Step<Real>::update;

  /** \brief Constructor.

      Standard constructor to build a ProjectedNewtonKrylovStep object.  Algorithmic 
      specifications are passed in through a Teuchos::ParameterList.

      @param[in]     parlist    is a parameter list containing algorithmic specifications
  */
  ProjectedNewtonKrylovStep( Teuchos::ParameterList &parlist )
    : Step<Real>(), secant_(Teuchos::null), krylov_(Teuchos::null),
      gp_(Teuchos::null), d_(Teuchos::null),
      hessian_(Teuchos::null), precond_(Teuchos::null),
      iterKrylov_(0), flagKrylov_(0), verbosity_(0), useSecantPrecond_(false) {
    // Parse ParameterList
    Teuchos::ParameterList& Glist = parlist.sublist("General");
    useSecantPrecond_ = Glist.sublist("Secant").get("Use as Preconditioner", false);
    useProjectedGrad_ = Glist.get("Projected Gradient Criticality Measure", false);
    verbosity_ = Glist.get("Print Verbosity",0);
    // Initialize Krylov object
    ekv_ = StringToEKrylov(Glist.sublist("Krylov").get("Type","Conjugate Gradients"));
    krylov_ = KrylovFactory<Real>(parlist);
    // Initialize secant object
    esec_ = StringToESecant(Glist.sublist("Secant").get("Type","Limited-Memory BFGS"));
    if ( useSecantPrecond_ ) {
      secant_ = SecantFactory<Real>(parlist);
    }
  }

  /** \brief Constructor.

      Constructor to build a ProjectedNewtonKrylovStep object with a user-defined 
      secant object.  Algorithmic specifications are passed in through 
      a Teuchos::ParameterList.

      @param[in]     parlist    is a parameter list containing algorithmic specifications
      @param[in]     secant     is a user-defined secant object
  */
//  ProjectedNewtonKrylovStep(Teuchos::ParameterList &parlist,
//             const Teuchos::RCP<Secant<Real> > &secant)
//    : Step<Real>(), secant_(secant), krylov_(Teuchos::null),
//      esec_(SECANT_USERDEFINED),
//      gp_(Teuchos::null), d_(Teuchos::null),
//      hessian_(Teuchos::null), precond_(Teuchos::null),
//      iterKrylov_(0), flagKrylov_(0), verbosity_(0), useSecantPrecond_(false) {
//    // Parse ParameterList
//    Teuchos::ParameterList& Glist = parlist.sublist("General");
//    useSecantPrecond_ = Glist.sublist("Secant").get("Use as Preconditioner", false);
//    useProjectedGrad_ = Glist.get("Projected Gradient Criticality Measure", false);
//    verbosity_ = Glist.get("Print Verbosity",0);
//    // Initialize secant object
//    if ( useSecantPrecond_ && secant_ == Teuchos::null ) {
//      esec_ = StringToESecant(Glist.sublist("Secant").get("Type","Limited-Memory BFGS"));
//      secant_ = SecantFactory<Real>(parlist);
//    }
//    // Initialize Krylov object
//    ekv_ = StringToEKrylov(Glist.sublist("Krylov").get("Type","Conjugate Gradients"));
//    krylov_ = KrylovFactory<Real>(parlist);
//  }
//
//  /** \brief Constructor.
//
//      Constructor to build a ProjectedNewtonKrylovStep object with a user-defined
//      Krylov object.  Algorithmic specifications are passed in through a
//      Teuchos::ParameterList.
//
//      @param[in]     parlist    is a parameter list containing algorithmic specifications
//      @param[in]     krylov     is a user-defined Krylov object
//  */
//  ProjectedNewtonKrylovStep(Teuchos::ParameterList &parlist,
//             const Teuchos::RCP<Krylov<Real> > &krylov)
//    : Step<Real>(), secant_(Teuchos::null), krylov_(krylov),
//      ekv_(KRYLOV_USERDEFINED),
//      gp_(Teuchos::null), d_(Teuchos::null),
//      hessian_(Teuchos::null), precond_(Teuchos::null),
//      iterKrylov_(0), flagKrylov_(0), verbosity_(0), useSecantPrecond_(false) {
//    // Parse ParameterList
//    Teuchos::ParameterList& Glist = parlist.sublist("General");
//    useProjectedGrad_ = Glist.get("Projected Gradient Criticality Measure", false);
//    useSecantPrecond_ = Glist.sublist("Secant").get("Use as Preconditioner", false);
//    verbosity_ = Glist.get("Print Verbosity",0);
//    // Initialize secant object
//    esec_ = StringToESecant(Glist.sublist("Secant").get("Type","Limited-Memory BFGS"));
//    if ( useSecantPrecond_ ) {
//      secant_ = SecantFactory<Real>(parlist);
//    }
//    // Initialize Krylov object
//    if ( krylov_ == Teuchos::null ) {
//      ekv_ = StringToEKrylov(Glist.sublist("Krylov").get("Type","Conjugate Gradients"));
//      krylov_ = KrylovFactory<Real>(parlist);
//    }
//  }

  /** \brief Constructor.

      Constructor to build a ProjectedNewtonKrylovStep object with user-defined 
      secant and Krylov objects.  Algorithmic specifications are passed in through 
      a Teuchos::ParameterList.

      @param[in]     parlist    is a parameter list containing algorithmic specifications
      @param[in]     krylov     is a user-defined Krylov object
      @param[in]     secant     is a user-defined secant object
  */
  ProjectedNewtonKrylovStep(Teuchos::ParameterList &parlist,
             const Teuchos::RCP<Krylov<Real> > &krylov,
             const Teuchos::RCP<Secant<Real> > &secant)
    : Step<Real>(), secant_(secant), krylov_(krylov),
      ekv_(KRYLOV_USERDEFINED), esec_(SECANT_USERDEFINED),
      gp_(Teuchos::null), d_(Teuchos::null),
      hessian_(Teuchos::null), precond_(Teuchos::null),
      iterKrylov_(0), flagKrylov_(0), verbosity_(0), useSecantPrecond_(false) {
    // Parse ParameterList
    Teuchos::ParameterList& Glist = parlist.sublist("General");
    useSecantPrecond_ = Glist.sublist("Secant").get("Use as Preconditioner", false);
    useProjectedGrad_ = Glist.get("Projected Gradient Criticality Measure", false);
    verbosity_ = Glist.get("Print Verbosity",0);
    // Initialize secant object
    if ( useSecantPrecond_ && secant_ == Teuchos::null ) {
      esec_ = StringToESecant(Glist.sublist("Secant").get("Type","Limited-Memory BFGS"));
      secant_ = SecantFactory<Real>(parlist);
    }
    // Initialize Krylov object
    if ( krylov_ == Teuchos::null ) {
      ekv_ = StringToEKrylov(Glist.sublist("Krylov").get("Type","Conjugate Gradients"));
      krylov_ = KrylovFactory<Real>(parlist);
    }
  }

  void initialize( Vector<Real> &x, const Vector<Real> &s, const Vector<Real> &g,
                   Objective<Real> &obj, BoundConstraint<Real> &bnd,
                   AlgorithmState<Real> &algo_state ) {
    Step<Real>::initialize(x,s,g,obj,bnd,algo_state);
    gp_ = g.clone();
    d_  = s.clone();
  }

  void compute( Vector<Real> &s, const Vector<Real> &x,
                Objective<Real> &obj, BoundConstraint<Real> &bnd,
                AlgorithmState<Real> &algo_state ) {
    Teuchos::RCP<StepState<Real> > step_state = Step<Real>::getState();

    // Build Hessian and Preconditioner object
    Teuchos::RCP<Objective<Real> > obj_ptr = Teuchos::rcp(&obj, false);
    Teuchos::RCP<BoundConstraint<Real> > bnd_ptr = Teuchos::rcp(&bnd, false);
    hessian_ = Teuchos::rcp(new HessianPNK(obj_ptr,bnd_ptr,
      algo_state.iterateVec,step_state->gradientVec,algo_state.gnorm));
    if (useSecantPrecond_) {
      precond_ = Teuchos::rcp(new PrecondPNK(secant_,bnd_ptr,
        algo_state.iterateVec,step_state->gradientVec,algo_state.gnorm));
    }
    else {
      precond_ = Teuchos::rcp(new PrecondPNK(obj_ptr,bnd_ptr,
        algo_state.iterateVec,step_state->gradientVec,algo_state.gnorm));
    }

    // Run Krylov method
    flagKrylov_ = 0;
    krylov_->run(s,*hessian_,*(step_state->gradientVec),*precond_,iterKrylov_,flagKrylov_);

    // Check Krylov flags
    if ( flagKrylov_ == 2 && iterKrylov_ <= 1 ) {
      s.set((step_state->gradientVec)->dual());
    }
    s.scale(-1.0);
  }

  void update( Vector<Real> &x, const Vector<Real> &s,
               Objective<Real> &obj, BoundConstraint<Real> &bnd,
               AlgorithmState<Real> &algo_state ) {
    Real tol = std::sqrt(ROL_EPSILON);
    Teuchos::RCP<StepState<Real> > step_state = Step<Real>::getState();

    // Update iterate and store previous step
    algo_state.iter++;
    d_->set(x);
    x.axpy(1.0, s);
    bnd.project(x);
    (step_state->descentVec)->set(x);
    (step_state->descentVec)->axpy(-1.0,*d_);
    algo_state.snorm = s.norm();

    // Compute new gradient
    if ( useSecantPrecond_ ) {
      gp_->set(*(step_state->gradientVec));
    }
    obj.update(x,true,algo_state.iter);
    algo_state.value = obj.value(x,tol);
    obj.gradient(*(step_state->gradientVec),x,tol);
    algo_state.ngrad++;

    // Update Secant Information
    if ( useSecantPrecond_ ) {
      secant_->updateStorage(x,*(step_state->gradientVec),*gp_,s,algo_state.snorm,algo_state.iter+1);
    }

    // Update algorithm state
    (algo_state.iterateVec)->set(x);
    if ( useProjectedGrad_ ) {
      gp_->set(*(step_state->gradientVec));
      bnd.computeProjectedGradient( *gp_, x );
      algo_state.gnorm = gp_->norm();
    }
    else {
      d_->set(x);
      d_->axpy(-1.0,(step_state->gradientVec)->dual());
      bnd.project(*d_);
      d_->axpy(-1.0,x);
      algo_state.gnorm = d_->norm();
    }
  }

  std::string printHeader( void ) const {
    std::stringstream hist;

    if( verbosity_>0 ) {
      hist << std::string(109,'-') <<  "\n";
      hist << EDescentToString(DESCENT_NEWTONKRYLOV);
      hist << " status output definitions\n\n";
      hist << "  iter     - Number of iterates (steps taken) \n";
      hist << "  value    - Objective function value \n";
      hist << "  gnorm    - Norm of the gradient\n";
      hist << "  snorm    - Norm of the step (update to optimization vector)\n";
      hist << "  #fval    - Cumulative number of times the objective function was evaluated\n";
      hist << "  #grad    - Number of times the gradient was computed\n";
      hist << "  iterCG   - Number of Krylov iterations used to compute search direction\n";
      hist << "  flagCG   - Krylov solver flag" << "\n";
      hist << std::string(109,'-') << "\n";
    }

    hist << "  ";
    hist << std::setw(6)  << std::left << "iter";
    hist << std::setw(15) << std::left << "value";
    hist << std::setw(15) << std::left << "gnorm";
    hist << std::setw(15) << std::left << "snorm";
    hist << std::setw(10) << std::left << "#fval";
    hist << std::setw(10) << std::left << "#grad";
    hist << std::setw(10) << std::left << "iterCG";
    hist << std::setw(10) << std::left << "flagCG";
    hist << "\n";
    return hist.str();
  }
  std::string printName( void ) const {
    std::stringstream hist;
    hist << "\n" << EDescentToString(DESCENT_NEWTONKRYLOV);
    hist << " using " << EKrylovToString(ekv_);
    if ( useSecantPrecond_ ) {
      hist << " with " << ESecantToString(esec_) << " preconditioning";
    }
    hist << "\n";
    return hist.str();
  }
  std::string print( AlgorithmState<Real> &algo_state, bool print_header = false ) const {
    std::stringstream hist;
    hist << std::scientific << std::setprecision(6);
    if ( algo_state.iter == 0 ) {
      hist << printName();
    }
    if ( print_header ) {
      hist << printHeader();
    }
    if ( algo_state.iter == 0 ) {
      hist << "  ";
      hist << std::setw(6) << std::left << algo_state.iter;
      hist << std::setw(15) << std::left << algo_state.value;
      hist << std::setw(15) << std::left << algo_state.gnorm;
      hist << "\n";
    }
    else {
      hist << "  ";
      hist << std::setw(6)  << std::left << algo_state.iter;
      hist << std::setw(15) << std::left << algo_state.value;
      hist << std::setw(15) << std::left << algo_state.gnorm;
      hist << std::setw(15) << std::left << algo_state.snorm;
      hist << std::setw(10) << std::left << algo_state.nfval;
      hist << std::setw(10) << std::left << algo_state.ngrad;
      hist << std::setw(10) << std::left << iterKrylov_;
      hist << std::setw(10) << std::left << flagKrylov_;
      hist << "\n";
    }
    return hist.str();
  }
}; // class ProjectedNewtonKrylovStep

} // namespace ROL

#endif
