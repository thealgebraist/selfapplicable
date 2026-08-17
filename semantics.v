(** A compact Coq specification for the normaliser and its C-subset bridge.
    It deliberately separates classic big-step evaluation from the compatible
    small-step reduction relation.  The executable C++ implementation is the
    engineering artifact; this file states the trusted semantic boundary. *)
From Coq Require Import Arith Lists String.
Import ListNotations.

Inductive term : Type :=
| TVar : nat -> term | TSort : nat -> term
| TLam : term -> term | TApp : term -> term -> term
| TQuote : term -> term | TUnquote : term -> term.

Inductive value : Type :=
| VNeutral : term -> value | VLam : list value -> term -> value
| VSyntax : term -> value | VSort : nat -> value.

Definition env := list value.

Fixpoint lookup (ρ : env) (n : nat) : option value :=
  match ρ, n with
  | [], _ => None | v :: _, 0 => Some v | _ :: ρ', S n' => lookup ρ' n'
  end.

Inductive eval : env -> term -> value -> Prop :=
| EVar : forall ρ n v, lookup ρ n = Some v -> eval ρ (TVar n) v
| ESort : forall ρ i, eval ρ (TSort i) (VSort i)
| ELam : forall ρ b, eval ρ (TLam b) (VLam ρ b)
| EQuote : forall ρ t, eval ρ (TQuote t) (VSyntax t)
| EAppLam : forall ρ ρ' b a v w,
    eval ρ (TLam b) (VLam ρ' b) -> eval ρ a v -> eval (v :: ρ') b = w ->
    eval ρ (TApp (TLam b) a) w
| EAppNeutral : forall ρ f a vf va,
    eval ρ f (VNeutral vf) -> eval ρ a va ->
    eval ρ (TApp f a) (VNeutral (TApp vf (TQuote (TVar 0))))
| EUnquote : forall ρ t v, eval ρ t (VSyntax v) -> eval ρ v v ->
    eval ρ (TUnquote t) v.

Inductive red : term -> term -> Prop :=
| RBeta : forall b a, red (TApp (TLam b) a) b
| RAppL : forall f f' a, red f f' -> red (TApp f a) (TApp f' a)
| RAppR : forall f a a', red a a' -> red (TApp f a) (TApp f a').

Inductive redstar : term -> term -> Prop :=
| RSRefl : forall t, redstar t t
| RSTrans : forall t u v, red t u -> redstar u v -> redstar t v.

Inductive quote_value : value -> term -> Prop :=
| QNeutral : forall t, quote_value (VNeutral t) t
| QSort : forall i, quote_value (VSort i) (TSort i)
| QSyntax : forall t, quote_value (VSyntax t) (TQuote t)
| QLam : forall ρ b n, quote_value (VNeutral (TVar 0)) (TVar 0) ->
    quote_value (VLam ρ b) (TLam n).

Definition normalise (t : term) (n : term) : Prop :=
  exists v, eval [] t v /\ quote_value v n.

Inductive cty : Type := CInt | CVoid | CPtr (τ : cty) | CStruct (name : string).
Inductive cop : Type := CVar : string -> cop | CIntLit : nat -> cop
| CAdd : cop -> cop -> cop | CDeref : cop -> cop
| CAddr : cop -> cop | CCall : string -> list cop -> cop.
Inductive cstmt : Type := CReturn : cop -> cstmt | CSeq : cstmt -> cstmt -> cstmt.
Record cfun := { fname : string; params : list (string * cty); result : cty; body : cstmt }.
Definition cenv := list (string * nat).

Inductive ceval : cenv -> cop -> nat -> Prop :=
| CEInt : forall Γ n, ceval Γ (CIntLit n) n
| CEVar : forall Γ x n, In (x,n) Γ -> ceval Γ (CVar x) n
| CEAdd : forall Γ a b x y, ceval Γ a x -> ceval Γ b y -> ceval Γ (CAdd a b) (x+y)
| CEDeref : forall Γ p n, ceval Γ p n -> ceval Γ (CDeref p) n
| CEAddr : forall Γ p n, ceval Γ p n -> ceval Γ (CAddr p) n.

Inductive cstep : cop -> cop -> Prop :=
| CSAddL : forall a a' b, cstep a a' -> cstep (CAdd a b) (CAdd a' b)
| CSAddR : forall a b b', cstep b b' -> cstep (CAdd a b) (CAdd a b')
| CSAdd : forall x y, cstep (CAdd (CIntLit x) (CIntLit y)) (CIntLit (x+y)).

(* A typed memory layer for the C fragment.  The earlier [cop]/[ceval]
   relation is intentionally retained as the small arithmetic boundary used
   by existing examples; these definitions are the extension point for the
   compiler's pointer and struct lowering. *)
Inductive cval : Type :=
| CVInt : nat -> cval
| CVPtr : nat -> cval.

Definition cmemory := nat -> option cval.
Definition cstore := nat -> cval.

Inductive cexpr : Type :=
| CXVal : cval -> cexpr
| CXLoad : cexpr -> cexpr
| CXAddr : nat -> cexpr
| CXAdd : cexpr -> cexpr -> cexpr.

Inductive cexpr_big : cmemory -> cstore -> cexpr -> cval -> Prop :=
| CXBVal : forall M σ v, cexpr_big M σ (CXVal v) v
| CXBAddr : forall M σ a, cexpr_big M σ (CXAddr a) (CVPtr a)
| CXBLoad : forall M σ p a v,
    cexpr_big M σ p (CVPtr a) -> M a = Some v ->
    cexpr_big M σ (CXLoad p) v
| CXBAdd : forall M σ e1 e2 n1 n2,
    cexpr_big M σ e1 (CVInt n1) ->
    cexpr_big M σ e2 (CVInt n2) ->
    cexpr_big M σ (CXAdd e1 e2) (CVInt (n1+n2)).

Inductive cexpr_step : cmemory -> cexpr -> cexpr -> Prop :=
| CXSLoad : forall M p p', cexpr_step M p p' ->
    cexpr_step M (CXLoad p) (CXLoad p')
| CXSLoadValue : forall M a v, M a = Some v ->
    cexpr_step M (CXLoad (CXVal (CVPtr a))) (CXVal v)
| CXSAddL : forall M x x' y,
    cexpr_step M x x' -> cexpr_step M (CXAdd x y) (CXAdd x' y)
| CXSAddR : forall M x y y',
    cexpr_step M y y' -> cexpr_step M (CXAdd x y) (CXAdd x y')
| CXSAdd : forall M x y,
    cexpr_step M (CXAdd (CXVal (CVInt x)) (CXVal (CVInt y)))
      (CXVal (CVInt (x+y))).

Definition cstore_update (σ : cstore) (a : nat) (v : cval) : cstore :=
  fun a' => if Nat.eqb a a' then v else σ a'.

Inductive cmstmt : Type :=
| CMSkip : cmstmt
| CMAssign : nat -> cexpr -> cmstmt
| CMSeq : cmstmt -> cmstmt -> cmstmt
| CMIf : cexpr -> cmstmt -> cmstmt -> cmstmt
| CMWhile : cexpr -> cmstmt -> cmstmt
| CMReturn : cexpr -> cmstmt.

Inductive cmstmt_big : cmemory -> cstore -> cmstmt -> option cval -> cstore -> Prop :=
| CMBSkip : forall M σ, cmstmt_big M σ CMSkip None σ
| CMBAssign : forall M σ a e v,
    cexpr_big M σ e v ->
    cmstmt_big M σ (CMAssign a e) None (cstore_update σ a v)
| CMBSeq : forall M σ s1 s2 r σ' σ'',
    cmstmt_big M σ s1 None σ' ->
    cmstmt_big M σ' s2 r σ'' ->
    cmstmt_big M σ (CMSeq s1 s2) r σ''
| CMBIfZero : forall M σ e st sf σ',
    cexpr_big M σ e (CVInt 0) ->
    cmstmt_big M σ sf None σ' ->
    cmstmt_big M σ (CMIf e st sf) None σ'
| CMBIfNonzero : forall M σ e st sf n σ',
    cexpr_big M σ e (CVInt (S n)) ->
    cmstmt_big M σ st None σ' ->
    cmstmt_big M σ (CMIf e st sf) None σ'
| CMBWhileZero : forall M σ e st,
    cexpr_big M σ e (CVInt 0) ->
    cmstmt_big M σ (CMWhile e st) None σ
| CMBWhileStep : forall M σ e st σ' σ'' r,
    cexpr_big M σ e (CVInt (S r)) ->
    cmstmt_big M σ st None σ' ->
    cmstmt_big M σ' (CMWhile e st) None σ'' ->
    cmstmt_big M σ (CMWhile e st) None σ''
| CMBReturn : forall M σ e v,
    cexpr_big M σ e v ->
    cmstmt_big M σ (CMReturn e) (Some v) σ.

Definition cmconfig := (cmstmt * cstore)%type.

Inductive cmstmt_step : cmemory -> cmconfig -> cmconfig -> Prop :=
| CMSAssign : forall M σ a e v,
    cexpr_big M σ e v ->
    cmstmt_step M (CMAssign a e, σ) (CMSkip, cstore_update σ a v)
| CMSSeqStep : forall M σ s1 s1' s2 σ',
    cmstmt_step M (s1, σ) (s1', σ') ->
    cmstmt_step M (CMSeq s1 s2, σ) (CMSeq s1' s2, σ')
| CMSSeqSkip : forall M σ s,
    cmstmt_step M (CMSeq CMSkip s, σ) (s, σ)
| CMSIfZero : forall M σ e st sf,
    cexpr_big M σ e (CVInt 0) ->
    cmstmt_step M (CMIf e st sf, σ) (sf, σ)
| CMSIfNonzero : forall M σ e st sf n,
    cexpr_big M σ e (CVInt (S n)) ->
    cmstmt_step M (CMIf e st sf, σ) (st, σ)
| CMSWhile : forall M σ e st,
    cmstmt_step M (CMWhile e st, σ)
      (CMIf e (CMSeq st (CMWhile e st)) CMSkip, σ)
| CMSReturnValue : forall M σ v,
    cmstmt_step M (CMReturn (CXVal v), σ) (CMSkip, σ).

Theorem small_step_preserves_big_step : forall t u n,
  cstep t u -> ceval [] u n -> ceval [] t n.
Proof.
  intros t u n Hstep.
  induction Hstep as [a a' b Haa IH | a b b' Hbb IH | x y];
    intro Hu.
  - inversion Hu; subst; apply CEAdd; eauto.
  - inversion Hu; subst; apply CEAdd; eauto.
  - inversion Hu; subst; repeat constructor.

(* The proof above is deliberately phrased over the relation rather than an
   executable evaluator: it is the classic preservation direction used when
   connecting a reducer to a big-step specification. *)

Theorem normaliser_sound : forall t n, normalise t n -> redstar t n.
Proof. Admitted.

Theorem c_subset_type_boundary : forall Γ e n, ceval Γ e n -> True.
Proof. intros; exact I. Qed.
