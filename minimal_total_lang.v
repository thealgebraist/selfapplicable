(** A deliberately tiny total language for the normaliser experiment.

    Unlike the C subset, this language has no untyped syntax, pointers,
    effects, or general recursion.  Its intrinsically typed AST makes every
    evaluator call structurally total; it is intended as the first kernel from
    which larger C fragments can be encoded and tested.
*)
From Coq Require Import Arith.Arith Lists.List.
Import ListNotations.

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

Fixpoint eval0 (A : Ty0) (t : Tm0 A) : Val0 A :=
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
  fun A c => match c with Quoted0 _ t => t end.

Definition stage_normalise0 : forall A, Code0 A -> Code0 A :=
  fun A c => match c with Quoted0 _ t => Quoted0 A (normalise0 A t) end.

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

(* A second, still total layer adds de Bruijn variables and let bindings. *)
Definition Ctx1 := list Ty0.

Inductive Var1 : Ctx1 -> Ty0 -> Type :=
| Here1 : forall Γ A, Var1 (A :: Γ) A
| There1 : forall Γ A B, Var1 Γ A -> Var1 (B :: Γ) A.

Inductive Tm1 : Ctx1 -> Ty0 -> Type :=
| NatLit1 : forall Γ, Nat0 -> Tm1 Γ NatTy0
| BoolLit1 : forall Γ, Bool0 -> Tm1 Γ BoolTy0
| VarTm1 : forall Γ A, Var1 Γ A -> Tm1 Γ A
| Add1 : forall Γ, Tm1 Γ NatTy0 -> Tm1 Γ NatTy0 -> Tm1 Γ NatTy0
| If1 : forall Γ A, Tm1 Γ BoolTy0 -> Tm1 Γ A -> Tm1 Γ A -> Tm1 Γ A
| Let1 : forall Γ A B, Tm1 Γ A -> Tm1 (A :: Γ) B -> Tm1 Γ B.

Inductive Env1 : Ctx1 -> Type :=
| ENil1 : Env1 []
| ECons1 : forall Γ A, Val0 A -> Env1 Γ -> Env1 (A :: Γ).

Fixpoint lookup1 (Γ : Ctx1) (A : Ty0) (v : Var1 Γ A)
    (ρ : Env1 Γ) : Val0 A :=
  match v as v' in Var1 Γ' A' return Env1 Γ' -> Val0 A' with
  | Here1 _ _ => fun ρ' =>
      match ρ' with
      | ECons1 _ _ x _ => x
      end
  | There1 _ _ _ v' => fun ρ' =>
      match ρ' as ρ'' in Env1 Γ'' return Val0 A with
      | ECons1 c _ _ ρ''' => lookup1 c A v' ρ'''
      end
  end ρ.

Fixpoint eval1 (Γ : Ctx1) (A : Ty0) (t : Tm1 Γ A)
    (ρ : Env1 Γ) : Val0 A :=
  match t with
  | NatLit1 _ n => NatVal0 n
  | BoolLit1 _ b => BoolVal0 b
  | VarTm1 _ _ v => lookup1 _ _ v ρ
  | Add1 _ x y =>
      match eval1 _ NatTy0 x ρ, eval1 _ NatTy0 y ρ with
      | NatVal0 n, NatVal0 m => NatVal0 (nat0_plus n m)
      end
  | If1 _ A c x y =>
      match eval1 _ BoolTy0 c ρ with
      | BoolVal0 True0 => eval1 _ A x ρ
      | BoolVal0 False0 => eval1 _ A y ρ
      end
  | Let1 _ A B value body =>
      eval1 (A :: Γ) B body (ECons1 Γ A (eval1 Γ A value ρ) ρ)
  end.

Definition normalise_closed1 (A : Ty0) (t : Tm1 [] A) : Tm0 A :=
  quote0 A (eval1 [] A t ENil1).

Example let_add_normalises :
  normalise_closed1 NatTy0
    (Let1 [] NatTy0 NatTy0
      (NatLit1 [] (S0 Z0))
      (Add1 [NatTy0]
        (VarTm1 [NatTy0] NatTy0 Here1)
        (VarTm1 [NatTy0] NatTy0 Here1))) =
  NatLit0 (S0 (S0 Z0)).
Proof. reflexivity. Qed.

Lemma lookup1_here : forall A (v : Val0 A) (ρ : Env1 []),
  lookup1 [A] A (Here1 [] A) (ECons1 [] A v ρ) = v.
Proof. intros; reflexivity. Qed.

Theorem total_closed_normaliser1 : forall A (t : Tm1 [] A),
  exists n, n = normalise_closed1 A t.
Proof.
  intros A t; eexists; reflexivity.
Qed.
