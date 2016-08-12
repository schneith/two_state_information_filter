/*
 * Model.hpp
 *
 *  Created on: Jul 29, 2016
 *      Author: Bloeschm
 */

#ifndef GIF_MODEL_HPP_
#define GIF_MODEL_HPP_

#include <list>
#include <initializer_list>

#include "generalized_information_filter/common.hpp"
#include "generalized_information_filter/StateDefinition.hpp"

namespace GIF{

template<typename... Ts>
class ElementPack{
 public:
  static constexpr int n_ = sizeof...(Ts);
  typedef std::tuple<Ts...> mtTuple;

  static std::shared_ptr<StateDefinition> makeStateDefinition(const std::array<std::string,n_>& names){
    std::shared_ptr<StateDefinition> def(new StateDefinition());
    addElementToDefinition(names,def);
    return def;
  }

  template<int i = 0, typename std::enable_if<(i<sizeof...(Ts))>::type* = nullptr>
  static void addElementToDefinition(const std::array<std::string,n_>& names, std::shared_ptr<StateDefinition> def){
    typedef typename std::tuple_element<i,mtTuple>::type mtElementType;
    def->addElementDefinition<mtElementType>(names.at(i));
    addElementToDefinition<i+1>(names,def);
  }
  template<int i = 0, typename std::enable_if<(i==sizeof...(Ts))>::type* = nullptr>
  static void addElementToDefinition(const std::array<std::string,n_>& names, std::shared_ptr<StateDefinition> def){}

  template<int i = 0, typename std::enable_if<(i<sizeof...(Ts))>::type* = nullptr>
  static void addElementToDefinition(std::shared_ptr<StateDefinition> def){
    typedef typename std::tuple_element<i,mtTuple>::type mtElementType;
    def->addElementDefinition<mtElementType>("def" + std::to_string(i));
    addElementToDefinition<i+1>(def);
  }
  template<int i = 0, typename std::enable_if<(i==sizeof...(Ts))>::type* = nullptr>
  static void addElementToDefinition(std::shared_ptr<StateDefinition> def){}
};

template<typename... InPacks>
struct TH_pack_size;
template<typename... Ts, typename... InPacks>
struct TH_pack_size<ElementPack<Ts...>,InPacks...>{
  static constexpr int n_ = TH_pack_size<InPacks...>::n_+sizeof...(Ts);
};
template<typename... Ts>
struct TH_pack_size<ElementPack<Ts...>>{
  static constexpr int n_ = sizeof...(Ts);
};

template<int i, typename... Packs>
struct TH_pack_index;
template<int i, typename... Ts, typename... Packs>
struct TH_pack_index<i,ElementPack<Ts...>,Packs...>{
  template<int j=i, typename std::enable_if<(j>=sizeof...(Ts))>::type* = nullptr>
  static constexpr int getOuter(){return TH_pack_index<i-sizeof...(Ts),Packs...>::getOuter()+1;}
  template<int j=i, typename std::enable_if<(j<sizeof...(Ts))>::type* = nullptr>
  static constexpr int getOuter(){return 0;}
  template<int j=i, typename std::enable_if<(j>=sizeof...(Ts))>::type* = nullptr>
  static constexpr int getInner(){return TH_pack_index<i-sizeof...(Ts),Packs...>::getInner();}
  template<int j=i, typename std::enable_if<(j<sizeof...(Ts))>::type* = nullptr>
  static constexpr int getInner(){return i;}
};
template<int i, typename... Ts>
struct TH_pack_index<i,ElementPack<Ts...>>{
  template<typename std::enable_if<(i<sizeof...(Ts))>::type* = nullptr>
  static constexpr int getOuter(){return 0;};
  template<typename std::enable_if<(i<sizeof...(Ts))>::type* = nullptr>
  static constexpr int getInner(){return i;};
};

template<typename Derived, typename OutPack, typename... InPacks>
class Model{
 public:
  static constexpr int n_ = OutPack::n_;
  static constexpr int m_ = TH_pack_size<InPacks...>::n_;
  static constexpr int N_ = sizeof...(InPacks);
  typedef Model<Derived,OutPack,InPacks...> mtBase;
  Model(const std::array<std::string,n_>& namesOut, const std::tuple<std::array<std::string,InPacks::n_>...>& namesIn){
    outDefinition_ = OutPack::makeStateDefinition(namesOut);
    makeInDefinitons(namesIn);
  }

  template<int i = 0, typename std::enable_if<(i<N_)>::type* = nullptr>
  void makeInDefinitons(const std::tuple<std::array<std::string,InPacks::n_>...>& namesIn){
    inDefinitions_[i] = std::tuple_element<i,std::tuple<InPacks...>>::type::makeStateDefinition(std::get<i>(namesIn));
    makeInDefinitons<i+1>(namesIn);
  }
  template<int i = 0, typename std::enable_if<(i==N_)>::type* = nullptr>
  void makeInDefinitons(const std::tuple<std::array<std::string,InPacks::n_>...>& namesIn){}

  template<typename... Ps, typename std::enable_if<(sizeof...(Ps)<n_)>::type* = nullptr>
  void _eval(const std::shared_ptr<State>& out, const std::array<const std::shared_ptr<const State>,N_>& ins, Ps&... elements) const{
    static constexpr int innerIndex = sizeof...(Ps);
    typedef typename OutPack::mtTuple mtTuple;
    typedef typename std::tuple_element<innerIndex,mtTuple>::type mtElementType;
    _eval(out, ins, elements..., dynamic_cast<Element<mtElementType>*>(out->getElement(innerIndex))->get());
  }
  template<typename... Ps, typename std::enable_if<(sizeof...(Ps)>=n_ & sizeof...(Ps)<n_+m_)>::type* = nullptr>
  void _eval(const std::shared_ptr<State>& out, const std::array<const std::shared_ptr<const State>,N_>& ins, Ps&... elements) const{
    static constexpr int outerIndex = TH_pack_index<sizeof...(Ps)-n_,InPacks...>::getOuter();
    static constexpr int innerIndex = TH_pack_index<sizeof...(Ps)-n_,InPacks...>::getInner();
    typedef typename std::tuple_element<outerIndex,std::tuple<InPacks...>>::type::mtTuple mtTuple;
    typedef typename std::tuple_element<innerIndex,mtTuple>::type mtElementType;
    _eval(out, ins, elements..., dynamic_cast<const Element<mtElementType>*>(ins.at(outerIndex)->getElement(innerIndex))->get());
  }
  template<typename... Ps, typename std::enable_if<(sizeof...(Ps)==m_+n_)>::type* = nullptr>
  void _eval(const std::shared_ptr<State>& out, const std::array<const std::shared_ptr<const State>,N_>& ins, Ps&... elements) const{
    static_cast<Derived&>(*this).eval(elements...);
  }

  template<int j, typename... Ps, typename std::enable_if<(sizeof...(Ps)<m_)>::type* = nullptr>
  void _jac(MXD& J, const std::array<const std::shared_ptr<const State>,N_>& ins, Ps&... elements) const{
    static constexpr int outerIndex = TH_pack_index<sizeof...(Ps),InPacks...>::getOuter();
    static constexpr int innerIndex = TH_pack_index<sizeof...(Ps),InPacks...>::getInner();
    typedef typename std::tuple_element<outerIndex,std::tuple<InPacks...>>::type::mtTuple mtTuple;
    typedef typename std::tuple_element<innerIndex,mtTuple>::type mtElementType;
    _jac<j>(J, ins, elements..., dynamic_cast<const Element<mtElementType>*>(ins.at(outerIndex)->getElement(innerIndex))->get());
  }
  template<int j, typename... Ps, typename std::enable_if<(sizeof...(Ps)==m_)>::type* = nullptr>
  void _jac(MXD& J, const std::array<const std::shared_ptr<const State>,N_>& ins, Ps&... elements) const{
    static_assert(j<N_,"No such Jacobian!");
    static_cast<Derived&>(*this).template jac<j>(J,elements...);
  }

  template<int j>
  void jacFD(MXD& J, const std::vector<const ElementBase*>& elementsIn) const{
    //  void _jacFD(const std::vector<const State*>& in, MXD& J, int c, const double& delta){
    //    J.setZero();
    //    State* stateDis = inputDefinitions_[c]->newState();
    //    *stateDis = *in[c];
    //    std::vector<const State*> inDis(in);
    //    inDis[c] = stateDis;
    //    State* outRef = outputDefinition_->newState();
    //    State* outDis = outputDefinition_->newState();
    //    _eval(inDis,outRef);
    //    VXD difIn(inputDefinitions_[c]->getDim());
    //    VXD difOut(outputDefinition_->getDim());
    //    for(int i=0; i<inputDefinitions_[c]->getDim(); i++){
    //      difIn.setZero();
    //      difIn(i) = delta;
    //      inputDefinitions_[c]->boxplus(in[c],difIn,stateDis);
    //      _eval(inDis,outDis);
    //      outputDefinition_->boxminus(outDis,outRef,difOut);
    //      J.col(i) = difOut/delta;
    //    }
    //    delete stateDis;
    //    delete outRef;
    //    delete outDis;
    //  }
  }

 protected:
  std::shared_ptr<StateDefinition> outDefinition_;
  std::array<std::shared_ptr<StateDefinition>,N_> inDefinitions_;
};

}

#endif /* GIF_MODEL_HPP_ */
