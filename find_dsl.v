(** A small Coq formalization of the Find DSL/compiler boundary.

    Strings and host filesystem effects are represented by finite abstract
    data here.  The theorem covers the language/checker/staging boundary; the
    C++ backend supplies the concrete filesystem traversal. *)
From Coq Require Import Bool.Bool Arith.PeanoNat.

Inductive FindKind : Type :=
| FKFile : FindKind
| FKDirectory : FindKind
| FKSymlink : FindKind.

Inductive SizeOp : Type :=
| SizeEq : SizeOp
| SizeGt : SizeOp
| SizeLt : SizeOp.

Inductive FindPred : Type :=
| FTrue : FindPred
| FName : nat -> FindPred
| FPath : nat -> FindPred
| FKind : FindKind -> FindPred
| FSize : SizeOp -> nat -> FindPred
| FEmpty : FindPred
| FPrune : FindPred
| FAnd : FindPred -> FindPred -> FindPred
| FOr : FindPred -> FindPred -> FindPred
| FNot : FindPred -> FindPred.

Record FindEntry : Type :=
  { entry_name : nat;
    entry_path : nat;
    entry_kind : FindKind;
    entry_size : nat;
    entry_empty : bool }.

Inductive CheckResult : Type :=
| Checked : CheckResult
| IllTyped : CheckResult.

Definition kind_eqb (a b : FindKind) : bool :=
  match a, b with
  | FKFile, FKFile | FKDirectory, FKDirectory | FKSymlink, FKSymlink => true
  | _, _ => false
  end.

Definition size_op_ok (op : SizeOp) : bool :=
  match op with SizeEq | SizeGt | SizeLt => true end.

Fixpoint check (p : FindPred) : bool :=
  match p with
  | FTrue => true
  | FName n => Nat.eqb n n
  | FPath n => Nat.eqb n n
  | FKind _ => true
  | FSize op _ => size_op_ok op
  | FEmpty | FPrune => true
  | FAnd p q | FOr p q => check p && check q
  | FNot p => check p
  end.

Definition size_test (op : SizeOp) (wanted actual : nat) : bool :=
  match op with
  | SizeEq => Nat.eqb actual wanted
  | SizeGt => wanted <? actual
  | SizeLt => actual <? wanted
  end.

Fixpoint eval (p : FindPred) (e : FindEntry) : bool :=
  match p with
  | FTrue => true
  | FName n => Nat.eqb n (entry_name e)
  | FPath n => Nat.eqb n (entry_path e)
  | FKind k => kind_eqb k (entry_kind e)
  | FSize op n => size_test op n (entry_size e)
  | FEmpty => entry_empty e
  | FPrune => true
  | FAnd p q => eval p e && eval q e
  | FOr p q => eval p e || eval q e
  | FNot p => negb (eval p e)
  end.

Definition has_prune (p : FindPred) : bool :=
  match p with
  | FPrune => true
  | _ => false
  end.

Inductive Code : Type :=
| Quoted : FindPred -> Code.

Definition quote (p : FindPred) : Code := Quoted p.
Definition unquote (c : Code) : FindPred :=
  match c with Quoted p => p end.

Definition normalize_code (c : Code) : option Code :=
  if check (unquote c) then Some c else None.

Definition compile_find (p : FindPred) : option FindPred :=
  match normalize_code (quote p) with
  | Some c => Some (unquote c)
  | None => None
  end.

Lemma check_total : forall p, exists b, check p = b.
Proof. intros; eexists; reflexivity. Qed.

Lemma quote_unquote : forall p, unquote (quote p) = p.
Proof. reflexivity. Qed.

Lemma compile_find_sound : forall p,
  check p = true -> compile_find p = Some p.
Proof.
  intros p H.
  unfold compile_find, normalize_code, quote, unquote.
  rewrite H. reflexivity.
Qed.

Definition example_find : FindPred :=
  FOr (FAnd (FName 7) FPrune) (FKind FKFile).

Example example_find_checked : check example_find = true.
Proof. reflexivity. Qed.

Example example_find_compiles : compile_find example_find = Some example_find.
Proof. apply compile_find_sound. reflexivity. Qed.

Example file_matches :
  eval example_find
    {| entry_name := 3; entry_path := 4; entry_kind := FKFile;
       entry_size := 10; entry_empty := false |} = true.
Proof. reflexivity. Qed.

Example prune_matches :
  eval example_find
    {| entry_name := 7; entry_path := 9; entry_kind := FKDirectory;
       entry_size := 0; entry_empty := false |} = true.
Proof. reflexivity. Qed.
