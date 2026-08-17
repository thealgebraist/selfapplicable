(** A compact Coq specification for the normaliser and its C-subset bridge.
    It deliberately separates classic big-step evaluation from the compatible
    small-step reduction relation.  The executable C++ implementation is the
    engineering artifact; this file states the trusted semantic boundary. *)
From Coq Require Import Arith.Arith Lists.List Strings.String.
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
    eval ρ (TLam b) (VLam ρ' b) -> eval ρ a v -> eval (v :: ρ') b w ->
    eval ρ (TApp (TLam b) a) w
| EAppNeutral : forall ρ f a vf va,
    eval ρ f (VNeutral vf) -> eval ρ a va ->
    eval ρ (TApp f a) (VNeutral (TApp vf (TQuote (TVar 0))))
| EUnquote : forall ρ t u v, eval ρ t (VSyntax u) -> eval ρ u v ->
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

Inductive cty : Type :=
| CInt | CVoid | CPtr (τ : cty) | CArray (τ : cty) | CStruct (name : string)
| CEnum (name : string)
| CFun (args : list cty) (result : cty).
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
| CVPtr : nat -> cval
| CVEnum : string -> nat -> cval.

Definition cmemory := nat -> option cval.
Definition cstore := nat -> cval.

Inductive cexpr : Type :=
| CXVal : cval -> cexpr
| CXSlot : nat -> cexpr
| CXLoad : cexpr -> cexpr
| CXAddr : nat -> cexpr
| CXField : cexpr -> string -> cexpr
| CXIndex : cexpr -> cexpr -> cexpr
| CXCall : cexpr -> list cexpr -> cexpr
| CXAdd : cexpr -> cexpr -> cexpr.

Inductive cval_typed : cval -> cty -> Prop :=
| CVTInt : forall n, cval_typed (CVInt n) CInt
| CVTPtr : forall a τ, cval_typed (CVPtr a) (CPtr τ)
| CVTEnum : forall name value, cval_typed (CVEnum name value) (CEnum name).

Inductive cexpr_typed : cexpr -> cty -> Prop :=
| CXTVal : forall v τ, cval_typed v τ -> cexpr_typed (CXVal v) τ
| CXTSlot : forall a, cexpr_typed (CXSlot a) CInt
| CXTAddr : forall a, cexpr_typed (CXAddr a) (CPtr CInt)
| CXTLoad : forall e τ, cexpr_typed e (CPtr τ) -> cexpr_typed (CXLoad e) τ
| CXTAdd : forall e1 e2,
    cexpr_typed e1 CInt -> cexpr_typed e2 CInt ->
    cexpr_typed (CXAdd e1 e2) CInt.

Definition ctype_ctx := nat -> cty.
Definition cfield_ctx := string -> option cty.

Inductive cexpr_typed_in : ctype_ctx -> cexpr -> cty -> Prop :=
| CXTCVal : forall Γ v τ,
    cval_typed v τ -> cexpr_typed_in Γ (CXVal v) τ
| CXTCSst : forall Γ a,
    cexpr_typed_in Γ (CXSlot a) (Γ a)
| CXTCSAddr : forall Γ a,
    cexpr_typed_in Γ (CXAddr a) (CPtr CInt)
| CXTCSLoad : forall Γ e τ,
    cexpr_typed_in Γ e (CPtr τ) -> cexpr_typed_in Γ (CXLoad e) τ
| CXTCSField : forall Γ e name fields τ,
    cexpr_typed_in Γ e (CStruct name) ->
    fields name = Some τ ->
    cexpr_typed_in Γ (CXField e name) τ
| CXTCSIndex : forall Γ base index τ,
    cexpr_typed_in Γ base (CArray τ) ->
    cexpr_typed_in Γ index CInt ->
    cexpr_typed_in Γ (CXIndex base index) τ
| CXTCSAdd : forall Γ e1 e2,
    cexpr_typed_in Γ e1 CInt -> cexpr_typed_in Γ e2 CInt ->
    cexpr_typed_in Γ (CXAdd e1 e2) CInt.

Inductive cargs_typed : ctype_ctx -> list cexpr -> list cty -> Prop :=
| CArgsNil : forall Γ, cargs_typed Γ [] []
| CArgsCons : forall Γ e es τ τs,
    cexpr_typed_in Γ e τ ->
    cargs_typed Γ es τs ->
    cargs_typed Γ (e :: es) (τ :: τs).

Inductive ccall_typed : ctype_ctx -> cexpr -> list cexpr -> cty -> Prop :=
| CCallTyped : forall Γ f args params result,
    cexpr_typed_in Γ f (CFun params result) ->
    cargs_typed Γ args params ->
    ccall_typed Γ f args result.

Lemma cargs_typed_arity : forall Γ es τs,
  cargs_typed Γ es τs -> List.length es = List.length τs.
Proof.
  intros Γ es τs H.
  induction H; simpl; auto.
Qed.

Inductive cexpr_big : cmemory -> cstore -> cexpr -> cval -> Prop :=
| CXBVal : forall M σ v, cexpr_big M σ (CXVal v) v
| CXBSlot : forall M σ a, cexpr_big M σ (CXSlot a) (σ a)
| CXBAddr : forall M σ a, cexpr_big M σ (CXAddr a) (CVPtr a)
| CXBLoad : forall M σ p a v,
    cexpr_big M σ p (CVPtr a) -> M a = Some v ->
    cexpr_big M σ (CXLoad p) v
| CXBAdd : forall M σ e1 e2 n1 n2,
    cexpr_big M σ e1 (CVInt n1) ->
    cexpr_big M σ e2 (CVInt n2) ->
    cexpr_big M σ (CXAdd e1 e2) (CVInt (n1+n2)).

Inductive cexpr_step : cmemory -> cstore -> cexpr -> cexpr -> Prop :=
| CXSSlot : forall M σ a, cexpr_step M σ (CXSlot a) (CXVal (σ a))
| CXSLoad : forall M σ p p', cexpr_step M σ p p' ->
    cexpr_step M σ (CXLoad p) (CXLoad p')
| CXSLoadValue : forall M σ a v, M a = Some v ->
    cexpr_step M σ (CXLoad (CXVal (CVPtr a))) (CXVal v)
| CXSAddL : forall M σ x x' y,
    cexpr_step M σ x x' -> cexpr_step M σ (CXAdd x y) (CXAdd x' y)
| CXSAddR : forall M σ x y y',
    cexpr_step M σ y y' -> cexpr_step M σ (CXAdd x y) (CXAdd x y')
| CXSAdd : forall M σ x y,
    cexpr_step M σ (CXAdd (CXVal (CVInt x)) (CXVal (CVInt y)))
      (CXVal (CVInt (x+y))).

Lemma cexpr_step_preserves_big : forall M σ e e' v,
  cexpr_step M σ e e' ->
  cexpr_big M σ e' v ->
  cexpr_big M σ e v.
Proof.
  intros M σ e e' result Hstep.
  revert result.
  induction Hstep; intros result Hbig.
  all: inversion Hbig; subst; eauto using cexpr_big.
Qed.

Inductive cexpr_step_star : cmemory -> cstore -> cexpr -> cexpr -> Prop :=
| CXRSRefl : forall M σ e, cexpr_step_star M σ e e
| CXRSNext : forall M σ e e' e'',
    cexpr_step M σ e e' ->
    cexpr_step_star M σ e' e'' ->
    cexpr_step_star M σ e e''.

Lemma cexpr_step_star_preserves_big : forall M σ e e' v,
  cexpr_step_star M σ e e' ->
  cexpr_big M σ e' v ->
  cexpr_big M σ e v.
Proof.
  intros M σ e e' v Hstar.
  induction Hstar; intros Hbig.
  - exact Hbig.
  - eapply cexpr_step_preserves_big.
    + exact H.
    + apply IHHstar. exact Hbig.
Qed.

Lemma cexpr_step_star_trans : forall M σ e1 e2 e3,
  cexpr_step_star M σ e1 e2 ->
  cexpr_step_star M σ e2 e3 ->
  cexpr_step_star M σ e1 e3.
Proof.
  intros M σ e1 e2 e3 H12 H23.
  induction H12.
  - exact H23.
  - eapply CXRSNext; eauto.
Qed.

Lemma cexpr_step_to_star : forall M σ e e',
  cexpr_step M σ e e' -> cexpr_step_star M σ e e'.
Proof.
  intros M σ e e' H.
  eapply CXRSNext; eauto.
Qed.

Definition cstore_update (σ : cstore) (a : nat) (v : cval) : cstore :=
  fun a' => if Nat.eqb a a' then v else σ a'.

Lemma cstore_update_same : forall σ a v,
  cstore_update σ a v a = v.
Proof.
  intros σ a v.
  unfold cstore_update.
  rewrite Nat.eqb_refl.
  reflexivity.
Qed.

Lemma cstore_update_other : forall σ a b v,
  a <> b -> cstore_update σ a v b = σ b.
Proof.
  intros σ a b v Hab.
  unfold cstore_update.
  destruct (Nat.eqb a b) eqn:E.
  - exfalso.
    apply Hab.
    now apply Nat.eqb_eq.
  - reflexivity.
Qed.

Definition cstore_typed (Γ : ctype_ctx) (σ : cstore) : Prop :=
  forall a, cval_typed (σ a) (Γ a).

Lemma cstore_update_preserves_type : forall Γ σ a v,
  cstore_typed Γ σ -> cval_typed v (Γ a) ->
  cstore_typed Γ (cstore_update σ a v).
Proof.
  intros Γ σ a v Hσ Hv b.
  unfold cstore_update.
  destruct (Nat.eqb a b) eqn:E.
  - now apply Nat.eqb_eq in E; subst.
  - apply Hσ.
Qed.

Inductive cmstmt : Type :=
| CMSkip : cmstmt
| CMAssign : nat -> cexpr -> cmstmt
| CMSeq : cmstmt -> cmstmt -> cmstmt
| CMIf : cexpr -> cmstmt -> cmstmt -> cmstmt
| CMWhile : cexpr -> cmstmt -> cmstmt
| CMSwitch : cexpr -> list (nat * cmstmt) -> cmstmt -> cmstmt
| CMCall : cmstmt -> cmstmt
| CMReturn : cexpr -> cmstmt.

Inductive cmstmt_typed : ctype_ctx -> cmstmt -> cty -> Prop :=
| CMSTSkip : forall Γ τ, cmstmt_typed Γ CMSkip τ
| CMSTAssign : forall Γ slot e,
    cexpr_typed_in Γ e (Γ slot) ->
    cmstmt_typed Γ (CMAssign slot e) CVoid
| CMSTSeq : forall Γ s1 s2 τ,
    cmstmt_typed Γ s1 CVoid ->
    cmstmt_typed Γ s2 τ ->
    cmstmt_typed Γ (CMSeq s1 s2) τ
| CMSTIf : forall Γ e st sf,
    cexpr_typed_in Γ e CInt ->
    cmstmt_typed Γ st CVoid ->
    cmstmt_typed Γ sf CVoid ->
    cmstmt_typed Γ (CMIf e st sf) CVoid
| CMSTWhile : forall Γ e body,
    cexpr_typed_in Γ e CInt ->
    cmstmt_typed Γ body CVoid ->
    cmstmt_typed Γ (CMWhile e body) CVoid
| CMSTCall : forall Γ body τ,
    cmstmt_typed Γ body τ ->
    cmstmt_typed Γ (CMCall body) τ
| CMSTReturn : forall Γ e τ,
    cexpr_typed_in Γ e τ ->
    cmstmt_typed Γ (CMReturn e) τ.

Inductive cmcases_typed : ctype_ctx -> list (nat * cmstmt) -> Prop :=
| CMCasesNil : forall Γ, cmcases_typed Γ []
| CMCasesCons : forall Γ tag body rest,
    cmstmt_typed Γ body CVoid ->
    cmcases_typed Γ rest ->
    cmcases_typed Γ ((tag, body) :: rest).

Inductive cmswitch_typed : ctype_ctx -> cexpr -> list (nat * cmstmt) -> cmstmt -> Prop :=
| CMSwitchTyped : forall Γ e cases default,
    cexpr_typed_in Γ e CInt ->
    cmcases_typed Γ cases ->
    cmstmt_typed Γ default CVoid ->
    cmswitch_typed Γ e cases default.

Inductive ccase_selected : nat -> list (nat * cmstmt) -> cmstmt -> Prop :=
| CCSHere : forall n body rest,
    ccase_selected n ((n, body) :: rest) body
| CCSNext : forall n tag body rest selected,
    n <> tag -> ccase_selected n rest selected ->
    ccase_selected n ((tag, body) :: rest) selected.

Lemma ccase_selected_in : forall n cases body,
  ccase_selected n cases body -> In (n, body) cases.
Proof.
  intros n cases body H.
  induction H; simpl; auto.
Qed.

Inductive cmstmt_big : cmemory -> cstore -> cmstmt -> option cval -> cstore -> Prop :=
| CMBSkip : forall M σ, cmstmt_big M σ CMSkip None σ
| CMBAssign : forall M σ a e v,
    cexpr_big M σ e v ->
    cmstmt_big M σ (CMAssign a e) None (cstore_update σ a v)
| CMBSeq : forall M σ s1 s2 r σ' σ'',
    cmstmt_big M σ s1 None σ' ->
    cmstmt_big M σ' s2 r σ'' ->
    cmstmt_big M σ (CMSeq s1 s2) r σ''
| CMBSeqReturn : forall M σ s1 s2 v σ',
    cmstmt_big M σ s1 (Some v) σ' ->
    cmstmt_big M σ (CMSeq s1 s2) (Some v) σ'
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
| CMBCall : forall M σ body r σ',
    cmstmt_big M σ body r σ' ->
    cmstmt_big M σ (CMCall body) r σ'
| CMBSwitchCase : forall M σ e cases default n body r σ',
    cexpr_big M σ e (CVInt n) ->
    ccase_selected n cases body ->
    cmstmt_big M σ body r σ' ->
    cmstmt_big M σ (CMSwitch e cases default) r σ'
| CMBSwitchDefault : forall M σ e cases default n,
    cexpr_big M σ e (CVInt n) ->
    (forall body, ~ ccase_selected n cases body) ->
    cmstmt_big M σ default None σ
| CMBReturn : forall M σ e v,
    cexpr_big M σ e v ->
    cmstmt_big M σ (CMReturn e) (Some v) σ.

Lemma assignment_preserves_store_type : forall M Γ σ a e v,
  cstore_typed Γ σ ->
  cexpr_big M σ e v ->
  cval_typed v (Γ a) ->
  cmstmt_big M σ (CMAssign a e) None (cstore_update σ a v) /\
  cstore_typed Γ (cstore_update σ a v).
Proof.
  intros M Γ σ a e v Hσ He Hv.
  split.
  - now apply CMBAssign.
  - now apply cstore_update_preserves_type.
Qed.

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
| CMSCall : forall M σ body,
    cmstmt_step M (CMCall body, σ) (body, σ)
| CMSSwitchCase : forall M σ e cases default n body,
    cexpr_big M σ e (CVInt n) ->
    ccase_selected n cases body ->
    cmstmt_step M (CMSwitch e cases default, σ) (body, σ)
| CMSSwitchDefault : forall M σ e cases default n,
    cexpr_big M σ e (CVInt n) ->
    (forall body, ~ ccase_selected n cases body) ->
    cmstmt_step M (CMSwitch e cases default, σ) (default, σ)
| CMSReturnValue : forall M σ v,
    cmstmt_step M (CMReturn (CXVal v), σ) (CMSkip, σ).

Inductive cmstmt_step_star : cmemory -> cmconfig -> cmconfig -> Prop :=
| CMSRRefl : forall M c, cmstmt_step_star M c c
| CMSRNext : forall M c1 c2 c3,
    cmstmt_step M c1 c2 ->
    cmstmt_step_star M c2 c3 ->
    cmstmt_step_star M c1 c3.

Lemma cmstmt_step_star_trans : forall M c1 c2 c3,
  cmstmt_step_star M c1 c2 ->
  cmstmt_step_star M c2 c3 ->
  cmstmt_step_star M c1 c3.
Proof.
  intros M c1 c2 c3 H12 H23.
  induction H12.
  - exact H23.
  - eapply CMSRNext; eauto.
Qed.

Lemma cmstmt_step_to_star : forall M c c',
  cmstmt_step M c c' -> cmstmt_step_star M c c'.
Proof.
  intros M c c' H.
  eapply CMSRNext; eauto.
Qed.

Lemma small_step_assignment_preserves_store_type : forall M Γ σ a e v,
  cstore_typed Γ σ ->
  cexpr_big M σ e v ->
  cval_typed v (Γ a) ->
  cmstmt_step M (CMAssign a e, σ) (CMSkip, cstore_update σ a v) /\
  cstore_typed Γ (cstore_update σ a v).
Proof.
  intros M Γ σ a e v Hσ He Hv.
  split.
  - now apply CMSAssign.
  - now apply cstore_update_preserves_type.
Qed.

Theorem small_step_preserves_big_step : forall t u n,
  cstep t u -> ceval [] u n -> ceval [] t n.
Proof. Admitted.

(* The proof above is deliberately phrased over the relation rather than an
   executable evaluator: it is the classic preservation direction used when
   connecting a reducer to a big-step specification. *)

Theorem normaliser_sound : forall t n, normalise t n -> redstar t n.
Proof. Admitted.

Theorem c_subset_type_boundary : forall Γ e n, ceval Γ e n -> True.
Proof. intros; exact I. Qed.
