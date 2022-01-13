// sona.js
// toki pona pi ilo sona.
//
// A toki pona inspired interperted language.

                                     //
               //                   /**/                   //
              /**/                  const                /**/
              SONA                  =()=>              {let c,
              v={},                 m={},              e= 0,
               l=0,                 M=[];             const
                la=                 "la",            li=//
                 "li",              nimi=           (x)=>(
                  String            )[""+        "fromC"+
                   "harC"+          "ode"       ](f(x)),



                               w = (X) =>(
                        X.match(/^[A-Z][a-zA-Z]*/)
                     !=undefined),B=(b)=>(X)=>{throw(
                 Error( `${b} ike: ${X} (${l})`))},E=B(
               "nimi"),R=(r)=>B("pali")(r),Q=B("toki"),W=
              (X)=>w(X)||E(X),P=(n,f)=>((...A)=>(A.length==
             n||R(...A))&&f(A[0])), p=(f)=>P(1, f),s=(s)=>(s
            ).split(/\s+/),f=(a)=>w(a)?v[a]:(a|0),u=(...A)=>(
           a=A.length)>3?Q(A):[f,(a,op)=>(o[op]||R(op))(f(a)),
          (a,op,b)=>(i[op]||R(op))(f(a),f(b))|0,][a-1](...A)|0,
          U=(p)=>(I[r=p.shift()]||R(r))(...p),n=(  )=>s(c[l++])
          ;const o={     "ala":(a)=>!a};const i    ={"anu":(a,b
          )=> (a| b),     "en": (a, b) => a+b,     "mute":(a, b
          ) =>  a * b,     "ante":(a, b)=>a-b     ,"weka":(a, b
          )=>a/b,li:(a,     b)=>a== b};const     I={"ken":(...L
          )=> u ( ... (L     ).splice((0),      L.indexOf( la))
           )&&L.shift()==       la&&U(L       ),"ijo" : (X ,L,
           ...Y)=>W(X) &&(L                 ==li||E(li))&&(v[
            X]=u(...Y)),"ma":p            ((ma)=>W(ma)&&(  m[
             ma]=l)),"tawa":p((a) =>W(a)&&(l=m[a])), "toki":
              (...X)=>M=M.concat( (X).map(nimi)), "nanpa":
               (...X)=>M=M.concat(X.map(f)),"pini":  P(0,
                 () =>(e=1)),"#":()=>{}};  return(S)=>{
                   c=S.split( '\n').filter ( (x)=>(x
                      ).length > 0);while(!e&&l<(c
                       ).length) U(n());return(
                               M ) ; } }


module.exports = SONA;
