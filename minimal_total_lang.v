(** A deliberately tiny total language for the normaliser experiment.

    Unlike the C subset, this language has no untyped syntax, pointers,
    effects, or general recursion.  Its intrinsically typed AST makes every
    evaluator call structurally total; it is intended as the first kernel from
    which larger C fragments can be encoded and tested.
*)
From Coq Require Import Arith.Arith.

Inductive Nat0 : Type :=
| Z0 : Nat0
| S0 : Nat0 -> Nat0.

Fixpoint nat0_plus (a b : Nat0) : Nat0 :=
  match a with
  | Z0 => b
  | S0 a' => S0 (nat0_plus a' b)
  end.

Inductive Bool0 : Type :=
| False0 : Bool0
| True0 : Bool0.

Inductive Ty0 : Type :=
| NatTy0 : Ty0
| BoolTy0 : Ty0.

Inductive Tm0 : Ty0 -> Type :=
| NatLit0 : Nat0 -> Tm0 NatTy0
| BoolLit0 : Bool0 -> Tm0 BoolTy0
| Add0 : Tm0 NatTy0 -> Tm0 NatTy0 -> Tm0 NatTy0
| If0 : forall A, Tm0 BoolTy0 -> Tm0 A -> Tm0 A -> Tm0 A.

Inductive Val0 : Ty0 -> Type :=
| NatVal0 : Nat0 -> Val0 NatTy0
| BoolVal0 : Bool0 -> Val0 BoolTy0.

Fixpoint eval0 : forall A, Tm0 A -> Val0 A :=
  fun A t =>
    match t with
    | NatLit0 n => NatVal0 n
    | BoolLit0 b => BoolVal0 b
    | Add0 x y =>
        match eval0 NatTy0 x, eval0 NatTy0 y with
        | NatVal0 n, NatVal0 m => NatVal0 (nat0_plus n m)
        end
    | If0 A c x y =>
        match eval0 BoolTy0 c with
        | BoolVal0 True0 => eval0 A x
        | BoolVal0 False0 => eval0 A y
        end
    end.

Definition quote0 : forall A, Val0 A -> Tm0 A :=
  fun A v =>
    match v with
    | NatVal0 n => NatLit0 n
    | BoolVal0 b => BoolLit0 b
    end.

Definition normalise0 : forall A, Tm0 A -> Tm0 A :=
  fun A t => quote0 A (eval0 A t).

Inductive Code0 (A : Ty0) : Type :=
| Quoted0 : Tm0 A -> Code0 A.

Definition unquote0 : forall A, Code0 A -> Tm0 A :=
  fun A c => match c with Quoted0 t => t end.

Definition stage_normalise0 : forall A, Code0 A -> Code0 A :=
  fun A c => match c with Quoted0 t => Quoted0 (normalise0 A t) end.

Lemma eval0_quote0 : forall A (v : Val0 A),
  eval0 A (quote0 A v) = v.
Proof.
  intros A v; destruct v; reflexivity.
Qed.

Lemma normalise0_is_value : forall A (t : Tm0 A),
  exists v, eval0 A t = v /\ normalise0 A t = quote0 A v.
Proof.
  intros A t; exists (eval0 A t); split; reflexivity.
Qed.

Lemma normalise0_idempotent : forall A (t : Tm0 A),
  normalise0 A (normalise0 A t) = normalise0 A t.
Proof.
  intros A t.
  unfold normalise0.
  rewrite eval0_quote0.
  reflexivity.
Qed.

Lemma stage_unquote_normalise : forall A (t : Tm0 A),
  unquote0 A (stage_normalise0 A (Quoted0 A t)) = normalise0 A t.
Proof.
  intros A t; reflexivity.
Qed.

Theorem total_normaliser : forall A (t : Tm0 A),
  exists n, n = normalise0 A t.
Proof.
  intros A t; eexists; reflexivity.
Qed.
