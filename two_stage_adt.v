(** A two-stage ADT language: the type stage is an ordinary total program. *)
From Coq Require Import Bool.Bool.

Inductive Ty2 : Type :=
| TNat2 : Ty2
| TBool2 : Ty2
| TPair2 : Ty2 -> Ty2 -> Ty2
| TCheckerTy2 : Ty2.

Inductive Checker2 : Type :=
| CNat2 : Checker2
| CBool2 : Checker2
| CAdd2 : Checker2 -> Checker2 -> Checker2
| CIf2 : Checker2 -> Checker2 -> Checker2 -> Checker2
| CPair2 : Checker2 -> Checker2 -> Checker2
| CFst2 : Checker2 -> Checker2
| CSnd2 : Checker2 -> Checker2
| CChecker2 : Checker2
| CRun2 : Checker2 -> Checker2
| CReject2 : Checker2.

Inductive Tm2 : Type :=
| Nat2 : nat -> Tm2
| Bool2 : bool -> Tm2
| Add2 : Tm2 -> Tm2 -> Tm2
| If2 : Tm2 -> Tm2 -> Tm2 -> Tm2
| Pair2 : Tm2 -> Tm2 -> Tm2
| Fst2 : Tm2 -> Tm2
| Snd2 : Tm2 -> Tm2
| CheckerCode2 : Checker2 -> Tm2
| Run2 : Tm2 -> Tm2 -> Tm2.

Inductive CheckResult2 : Type :=
| Accepted2 : Ty2 -> CheckResult2
| Rejected2 : CheckResult2.

Fixpoint ty_eqb2 (a b : Ty2) : bool :=
  match a, b with
  | TNat2, TNat2 => true
  | TBool2, TBool2 => true
  | TPair2 a1 a2, TPair2 b1 b2 => ty_eqb2 a1 b1 && ty_eqb2 a2 b2
  | TCheckerTy2, TCheckerTy2 => true
  | _, _ => false
  end.

Definition accept_if2 (a b : CheckResult2) : CheckResult2 :=
  match a, b with
  | Accepted2 ta, Accepted2 tb =>
      if ty_eqb2 ta tb then Accepted2 ta else Rejected2
  | _, _ => Rejected2
  end.

(* This is the user-written type stage.  The recursion is on the finite
   checker program, so every checker terminates even though the checker is
   supplied as data rather than built into the kernel. *)
Fixpoint run_type_stage2 (p : Checker2) (t : Tm2) : CheckResult2 :=
  match p with
  | CReject2 => Rejected2
  | CNat2 => match t with Nat2 _ => Accepted2 TNat2 | _ => Rejected2 end
  | CBool2 => match t with Bool2 _ => Accepted2 TBool2 | _ => Rejected2 end
  | CAdd2 p q =>
      match t with
      | Add2 x y =>
          match run_type_stage2 p x, run_type_stage2 q y with
          | Accepted2 TNat2, Accepted2 TNat2 => Accepted2 TNat2
          | _, _ => Rejected2
          end
      | _ => Rejected2
      end
  | CIf2 pc px py =>
      match t with
      | If2 c x y =>
          match run_type_stage2 pc c with
          | Accepted2 TBool2 => accept_if2 (run_type_stage2 px x) (run_type_stage2 py y)
          | _ => Rejected2
          end
      | _ => Rejected2
      end
  | CPair2 p q =>
      match t with
      | Pair2 x y =>
          match run_type_stage2 p x, run_type_stage2 q y with
          | Accepted2 tx, Accepted2 ty => Accepted2 (TPair2 tx ty)
          | _, _ => Rejected2
          end
      | _ => Rejected2
      end
  | CFst2 p =>
      match t with
      | Fst2 x =>
          match run_type_stage2 p x with
          | Accepted2 (TPair2 tx _) => Accepted2 tx
          | _ => Rejected2
          end
      | _ => Rejected2
      end
  | CSnd2 p =>
      match t with
      | Snd2 x =>
          match run_type_stage2 p x with
          | Accepted2 (TPair2 _ ty) => Accepted2 ty
          | _ => Rejected2
          end
      | _ => Rejected2
      end
  | CChecker2 =>
      match t with
      | CheckerCode2 _ => Accepted2 TCheckerTy2
      | _ => Rejected2
      end
  | CRun2 p =>
      match t with
      | Run2 code argument =>
          match code with
          | CheckerCode2 _ => run_type_stage2 p argument
          | _ => Rejected2
          end
      | _ => Rejected2
      end
  end.

Definition TypeStage2 := Checker2.
Definition typecheck2 (stage : TypeStage2) (program : Tm2) : CheckResult2 :=
  run_type_stage2 stage program.

Lemma type_stage_total2 : forall stage program,
  exists result, typecheck2 stage program = result.
Proof. intros; eexists; reflexivity. Qed.

Definition arithmetic_stage2 : Checker2 :=
  CAdd2 CNat2 CNat2.

Example arithmetic_stage_accepts2 :
  typecheck2 arithmetic_stage2 (Add2 (Nat2 2) (Nat2 3)) = Accepted2 TNat2.
Proof. reflexivity. Qed.

Definition pair_stage2 : Checker2 :=
  CPair2 CNat2 CBool2.

Example pair_stage_accepts2 :
  typecheck2 pair_stage2 (Pair2 (Nat2 9) (Bool2 true)) =
  Accepted2 (TPair2 TNat2 TBool2).
Proof. reflexivity. Qed.

(* The code boundary makes the type stage itself first-class.  CRun2 is a
   total, structurally recursive interpreter combinator: it executes the
   supplied checker on the argument carried by a quoted checker program. *)
Definition self_stage2 : Checker2 := CRun2 CChecker2.
Definition self_program2 : Tm2 :=
  Run2 (CheckerCode2 self_stage2) (CheckerCode2 self_stage2).

Example self_application2 :
  typecheck2 self_stage2 self_program2 = Accepted2 TCheckerTy2.
Proof. reflexivity. Qed.

(* The runtime is deliberately separate: it can run only after the caller has
   chosen and executed a type stage. *)
Inductive Val2 : Type :=
| VNat2 : nat -> Val2
| VBool2 : bool -> Val2
| VPair2 : Val2 -> Val2 -> Val2
| VChecker2 : Checker2 -> Val2.

Fixpoint eval2 (t : Tm2) : Val2 :=
  match t with
  | Nat2 n => VNat2 n
  | Bool2 b => VBool2 b
  | Add2 x y =>
      match eval2 x, eval2 y with
      | VNat2 n, VNat2 m => VNat2 (n + m)
      | _, _ => VNat2 0
      end
  | If2 c x y =>
      match eval2 c with
      | VBool2 true => eval2 x
      | VBool2 false => eval2 y
      | _ => VNat2 0
      end
  | Pair2 x y => VPair2 (eval2 x) (eval2 y)
  | Fst2 x => match eval2 x with VPair2 a _ => a | _ => VNat2 0 end
  | Snd2 x => match eval2 x with VPair2 _ b => b | _ => VNat2 0 end
  | CheckerCode2 c => VChecker2 c
  | Run2 _ _ => VNat2 0
  end.

Definition run2 (stage : TypeStage2) (program : Tm2) : option Val2 :=
  match typecheck2 stage program with
  | Accepted2 _ => Some (eval2 program)
  | Rejected2 => None
  end.

Example staged_run2 :
  run2 arithmetic_stage2 (Add2 (Nat2 2) (Nat2 3)) = Some (VNat2 5).
Proof. reflexivity. Qed.
