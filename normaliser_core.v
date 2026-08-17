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

Fixpoint nshift (d cutoff : nat) (t : nterm) : nterm :=
  match t with
  | NVar k => if cutoff <=? k then NVar (d + k) else NVar k
  | NLam body => NLam (nshift d (S cutoff) body)
  | NApp f a => NApp (nshift d cutoff f) (nshift d cutoff a)
  | NQuote code => NQuote (nshift d cutoff code)
  | NUnquote code => NUnquote (nshift d cutoff code)
  end.

Fixpoint nsubst (depth : nat) (replacement : nterm) (t : nterm) : nterm :=
  match t with
  | NVar k =>
      if k <? depth then NVar k
      else if k =? depth then nshift depth 0 replacement
      else NVar (pred k)
  | NLam body => NLam (nsubst (S depth) replacement body)
  | NApp f a => NApp (nsubst depth replacement f) (nsubst depth replacement a)
  | NQuote code => NQuote (nsubst depth replacement code)
  | NUnquote code => NUnquote (nsubst depth replacement code)
  end.

Definition nsubst0 (replacement : nterm) (t : nterm) : nterm :=
  nsubst 0 replacement t.

Lemma nsubst0_var0 : forall s,
  nsubst0 s (NVar 0) = s.
Proof.
  intros s.
  cbn [nsubst0 nsubst].
Qed.

Lemma nsubst0_var_succ : forall s k,
  nsubst0 s (NVar (S k)) = NVar k.
Proof.
  intros s k.
  cbn [nsubst0 nsubst].
  destruct k; reflexivity.
Qed.

Lemma nsubst0_under_bound_lambda : forall s,
  nsubst0 s (NLam (NVar 0)) = NLam (NVar 0).
Proof.
  intros s.
  cbn [nsubst0 nsubst].
Qed.

Lemma nsubst0_under_outer_lambda : forall s,
  nsubst0 s (NLam (NVar 1)) = NLam (nshift 1 0 s).
Proof.
  intros s.
  cbn [nsubst0 nsubst].
Qed.

(* The staged computational boundary.  Quotation is inert data; only an
   unquote of an immediately available quotation crosses the boundary. *)
Inductive nred : nterm -> nterm -> Prop :=
| NRBeta : forall body arg,
    nred (NApp (NLam body) arg) (nsubst0 arg body)
| NRQuoteUnquote : forall t,
    nred (NUnquote (NQuote t)) t
| NRAppLeft : forall f f' a,
    nred f f' -> nred (NApp f a) (NApp f' a)
| NRAppRight : forall f a a',
    nred a a' -> nred (NApp f a) (NApp f a')
| NRQuoteBody : forall t t',
    nred t t' -> nred (NQuote t) (NQuote t')
| NRLamBody : forall t t',
    nred t t' -> nred (NLam t) (NLam t')
| NRUnquote : forall c c',
    nred c c' -> nred (NUnquote c) (NUnquote c').

Lemma beta_identity : forall s,
  nred (NApp (NLam (NVar 0)) s) s.
Proof.
  intros s.
  change (nred (NApp (NLam (NVar 0)) s) (nsubst0 s (NVar 0))).
  constructor.
Qed.

Lemma beta_outer_variable : forall s,
  nred (NApp (NLam (NVar 1)) s) (NVar 0).
Proof.
  intros s.
  change (nred (NApp (NLam (NVar 1)) s) (nsubst0 s (NVar 1))).
  constructor.
Qed.

Inductive nred_star : nterm -> nterm -> Prop :=
| NRSRefl : forall t, nred_star t t
| NRSNext : forall t u v,
    nred t u -> nred_star u v -> nred_star t v.

Inductive nstage : nterm -> nterm -> Prop :=
| NSQuoteUnquote : forall t,
    nstage (NUnquote (NQuote t)) t.

Lemma nred_star_refl : forall t, nred_star t t.
Proof.
  intros t.
  constructor.
Qed.

Lemma nstage_is_nred : forall t u,
  nstage t u -> nred t u.
Proof.
  intros t u H.
  inversion H; subst.
  constructor.
Qed.

Lemma nstage_to_nred_star : forall t u,
  nstage t u -> nred_star t u.
Proof.
  intros t u Hstage.
  eapply NRSNext.
  - apply nstage_is_nred.
    exact Hstage.
  - apply nred_star_refl.
Qed.

Lemma nred_to_nred_star : forall t u,
  nred t u -> nred_star t u.
Proof.
  intros t u Hstep.
  eapply NRSNext.
  - exact Hstep.
  - apply nred_star_refl.
Qed.

Lemma nred_star_trans : forall t u v,
  nred_star t u -> nred_star u v -> nred_star t v.
Proof.
  intros t u v Htu Huv.
  induction Htu as [t | t u w Hstep Huw IH].
  - exact Huv.
  - eapply NRSNext.
    + exact Hstep.
    + apply IH.
      exact Huv.
Qed.

Lemma nstage_then_nred_star : forall t u v,
  nstage t u -> nred_star u v -> nred_star t v.
Proof.
  intros t u v Hstage Htail.
  apply nred_star_trans with (u := u).
  - apply nstage_to_nred_star.
    exact Hstage.
  - exact Htail.
Qed.

Lemma nred_star_quote : forall t u,
  nred_star t u -> nred_star (NQuote t) (NQuote u).
Proof.
  intros t u H.
  induction H as [t | t u v Hstep Htail IH].
  - apply nred_star_refl.
  - eapply NRSNext.
    + apply NRQuoteBody.
      exact Hstep.
    + exact IH.
Qed.

Lemma nred_star_unquote : forall t u,
  nred_star t u -> nred_star (NUnquote t) (NUnquote u).
Proof.
  intros t u H.
  induction H as [t | t u v Hstep Htail IH].
  - apply nred_star_refl.
  - eapply NRSNext.
    + apply NRUnquote.
      exact Hstep.
    + exact IH.
Qed.

Lemma nred_star_app_left : forall f f' a,
  nred_star f f' -> nred_star (NApp f a) (NApp f' a).
Proof.
  intros f f' a H.
  induction H as [f | f f' g Hstep Htail IH].
  - apply nred_star_refl.
  - eapply NRSNext.
    + apply NRAppLeft.
      exact Hstep.
    + exact IH.
Qed.

Lemma nred_star_app_right : forall f a a',
  nred_star a a' -> nred_star (NApp f a) (NApp f a').
Proof.
  intros f a a' H.
  induction H as [a | a a' b Hstep Htail IH].
  - apply nred_star_refl.
  - eapply NRSNext.
    + apply NRAppRight.
      exact Hstep.
    + exact IH.
Qed.

Lemma nred_star_lam : forall t u,
  nred_star t u -> nred_star (NLam t) (NLam u).
Proof.
  intros t u H.
  induction H as [t | t u v Hstep Htail IH].
  - apply nred_star_refl.
  - eapply NRSNext.
    + apply NRLamBody.
      exact Hstep.
    + exact IH.
Qed.

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

(* A sound normalization result carries the computational path separately
   from the typing and quotation facts.  Keeping the path explicit prevents
   the specification from silently treating quotation as evaluation. *)
Definition staged_normalise_result
    (Γ : list nty) (t : nterm) (A : nty) (n : nterm) : Prop :=
  ntyped Γ t A /\
  (exists v, nquote A v n) /\
  nred_star t n.

Lemma staged_normalise_result_typed : forall Γ t A n,
  staged_normalise_result Γ t A n -> ntyped Γ t A.
Proof.
  intros Γ t A n H.
  exact (proj1 H).
Qed.

Lemma staged_normalise_result_reaches : forall Γ t A n,
  staged_normalise_result Γ t A n -> nred_star t n.
Proof.
  intros Γ t A n H.
  exact (proj2 (proj2 H)).
Qed.

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

Lemma staged_round_trip : forall t,
  nred (NUnquote (NQuote t)) t.
Proof.
  intros t.
  constructor.
Qed.

Lemma staged_round_trip_star : forall t,
  nred_star (NUnquote (NQuote t)) t.
Proof.
  intros t.
  eapply NRSNext.
  - apply staged_round_trip.
  - constructor.
Qed.

Lemma staged_round_trip_typed : forall Γ t A,
  ntyped Γ t A ->
  exists u, nred (NUnquote (NQuote t)) u /\ ntyped Γ u A.
Proof.
  intros Γ t A H.
  exists t.
  split.
  - apply staged_round_trip.
  - exact H.
Qed.

Lemma staged_round_trip_preserves_type : forall Γ t A u,
  ntyped Γ t A ->
  nstage (NUnquote (NQuote t)) u ->
  ntyped Γ u A.
Proof.
  intros Γ t A u Htyped Hred.
  inversion Hred; subst.
  exact Htyped.
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
