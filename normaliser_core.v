(** A small, independently compilable specification of the typed staging
    boundary used by the executable normaliser. *)

From Coq Require Import Arith.Arith Lists.List.

Inductive nty : Type :=
| NType : nat -> nty
| NFun : nty -> nty -> nty
| NCode : nty -> nty.

Inductive nty_level : nty -> nat -> Prop :=
| NTLType : forall i, nty_level (NType i) (S i)
| NTLFun : forall A B i j,
    nty_level A i -> nty_level B j ->
    nty_level (NFun A B) (Nat.max i j)
| NTLCode : forall A i,
    nty_level A i -> nty_level (NCode A) i.

Inductive nterm : Type :=
| NVar : nat -> nterm
| NLam : nterm -> nterm
| NApp : nterm -> nterm -> nterm
| NQuote : nterm -> nterm
| NUnquote : nterm -> nterm.

Inductive ntyped : list nty -> nterm -> nty -> Prop :=
| NTVar : forall Γ n A,
    nth_error Γ n = Some A -> ntyped Γ (NVar n) A
| NTLam : forall Γ A body B,
    ntyped (A :: Γ) body B ->
    ntyped Γ (NLam body) (NFun A B)
| NTApp : forall Γ f a A B,
    ntyped Γ f (NFun A B) ->
    ntyped Γ a A ->
    ntyped Γ (NApp f a) B
| NTQuote : forall Γ t A,
    ntyped Γ t A -> ntyped Γ (NQuote t) (NCode A)
| NTUnquote : forall Γ t A,
    ntyped Γ t (NCode A) -> ntyped Γ (NUnquote t) A.

Inductive nneutral : Type :=
| NNVar : nat -> nneutral
| NNApp : nneutral -> nnormal -> nneutral
with nnormal : Type :=
| NNNeutral : nneutral -> nnormal
| NNLam : nnormal -> nnormal
| NNQuote : nnormal -> nnormal.

Inductive nquote : nty -> nnormal -> nterm -> Prop :=
| NQNeutral : forall A n, nquote A (NNNeutral n) (NVar 0)
| NQLam : forall A B body t, nquote B body t ->
    nquote (NFun A B) (NNLam body) (NLam t)
| NQQuote : forall A n t, nquote A n t -> nquote (NCode A) (NNQuote n) (NQuote t).

Definition staged_normalise (Γ : list nty) (t : nterm) (A : nty) (n : nterm) : Prop :=
  ntyped Γ t A /\ exists v, nquote A v n.

Lemma quote_preserves_type : forall Γ t A,
  ntyped Γ t A -> ntyped Γ (NQuote t) (NCode A).
Proof.
  intros Γ t A H.
  now apply NTQuote.
Qed.

Lemma unquote_preserves_type : forall Γ t A,
  ntyped Γ t (NCode A) -> ntyped Γ (NUnquote t) A.
Proof.
  intros Γ t A H.
  now apply NTUnquote.
Qed.

Lemma quote_unquote_type : forall Γ t A,
  ntyped Γ t A -> ntyped Γ (NUnquote (NQuote t)) A.
Proof.
  intros Γ t A H.
  apply NTUnquote.
  now apply NTQuote.
Qed.

Lemma neutral_has_quote : forall A k,
  exists t, nquote A (NNNeutral (NNVar k)) t.
Proof.
  intros A k.
  exists (NVar 0).
  constructor.
Qed.

Lemma application_preserves_result_type : forall Γ f a A B,
  ntyped Γ f (NFun A B) ->
  ntyped Γ a A ->
  ntyped Γ (NApp f a) B.
Proof.
  intros Γ f a A B Hf Ha.
  eapply NTApp; eauto.
Qed.

Lemma staged_normalise_has_type : forall Γ t A n,
  staged_normalise Γ t A n -> ntyped Γ t A.
Proof.
  intros Γ t A n H.
  exact (proj1 H).
Qed.
