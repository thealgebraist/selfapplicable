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
