(** Eight fixed-size rows, checked in stage one and calculated in stage two. *)
From Coq Require Import Bool.Bool.

Inductive Status4 : Type :=
| Low4 : Status4
| Medium4 : Status4
| High4 : Status4.

Inductive RawField4 : Type :=
| RawNat4 : nat -> RawField4
| RawBool4 : bool -> RawField4
| RawStatus4 : Status4 -> RawField4
| RawCount4 : nat -> RawField4.

Inductive RawRow4 : Type :=
| RawRow : RawField4 -> RawField4 -> RawField4 -> RawField4 -> RawRow4.

Record Row4 : Type :=
{ amount4 : nat
; enabled4 : bool
; status4 : Status4
; count4 : nat
}.

Inductive Array8 (A : Type) : Type :=
| Array8C : A -> A -> A -> A -> A -> A -> A -> A -> Array8 A.

Inductive RawArray8 : Type :=
| RawArray8C : RawRow4 -> RawRow4 -> RawRow4 -> RawRow4 ->
               RawRow4 -> RawRow4 -> RawRow4 -> RawRow4 -> RawArray8.

Definition option_bind {A B : Type} (x : option A) (f : A -> option B) : option B :=
  match x with
  | Some a => f a
  | None => None
  end.

(* This is the user-supplied type stage for one row.  The four constructor
   tags are checked explicitly; no implicit coercion is performed. *)
Definition check_row4 (r : RawRow4) : option Row4 :=
  match r with
  | RawRow (RawNat4 amount) (RawBool4 enabled)
            (RawStatus4 status) (RawCount4 count) =>
      Some {| amount4 := amount; enabled4 := enabled;
              status4 := status; count4 := count |}
  | _ => None
  end.

(* The array checker is also stage one.  It cannot return a typed Array8 Row4
   unless all eight raw rows pass check_row4. *)
Definition check_array8 (a : RawArray8) : option (Array8 Row4) :=
  match a with
  | RawArray8C r0 r1 r2 r3 r4 r5 r6 r7 =>
      option_bind (check_row4 r0) (fun v0 =>
      option_bind (check_row4 r1) (fun v1 =>
      option_bind (check_row4 r2) (fun v2 =>
      option_bind (check_row4 r3) (fun v3 =>
      option_bind (check_row4 r4) (fun v4 =>
      option_bind (check_row4 r5) (fun v5 =>
      option_bind (check_row4 r6) (fun v6 =>
      option_bind (check_row4 r7) (fun v7 =>
        Some (Array8C v0 v1 v2 v3 v4 v5 v6 v7))))))))
  end.

Definition status_score4 (s : Status4) : nat :=
  match s with Low4 => 1 | Medium4 => 2 | High4 => 3 end.

Definition row_score4 (r : Row4) : nat :=
  if enabled4 r
  then amount4 r * count4 r + status_score4 (status4 r)
  else 0.

Definition sum8 (a : Array8 nat) : nat :=
  match a with
  | Array8C a0 a1 a2 a3 a4 a5 a6 a7 =>
      a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7
  end.

Definition calculate4 (a : Array8 Row4) : nat :=
  match a with
  | Array8C r0 r1 r2 r3 r4 r5 r6 r7 =>
      sum8 (Array8C (row_score4 r0) (row_score4 r1)
                   (row_score4 r2) (row_score4 r3)
                   (row_score4 r4) (row_score4 r5)
                   (row_score4 r6) (row_score4 r7))
  end.

Definition run_array8 (a : RawArray8) : option nat :=
  match check_array8 a with
  | Some typed => Some (calculate4 typed)
  | None => None
  end.

Definition example_array8 : RawArray8 :=
  RawArray8C
    (RawRow (RawNat4 10) (RawBool4 true)  (RawStatus4 High4)   (RawCount4 2))
    (RawRow (RawNat4 3)  (RawBool4 false) (RawStatus4 Low4)    (RawCount4 9))
    (RawRow (RawNat4 4)  (RawBool4 true)  (RawStatus4 Medium4) (RawCount4 5))
    (RawRow (RawNat4 8)  (RawBool4 true)  (RawStatus4 Low4)    (RawCount4 1))
    (RawRow (RawNat4 2)  (RawBool4 true)  (RawStatus4 High4)   (RawCount4 4))
    (RawRow (RawNat4 7)  (RawBool4 false) (RawStatus4 Medium4) (RawCount4 3))
    (RawRow (RawNat4 6)  (RawBool4 true)  (RawStatus4 Medium4) (RawCount4 2))
    (RawRow (RawNat4 5)  (RawBool4 true)  (RawStatus4 Low4)    (RawCount4 2)).

Example array8_is_checked_and_calculated :
  run_array8 example_array8 = Some 90.
Proof. reflexivity. Qed.

Definition bad_array8 : RawArray8 :=
  RawArray8C
    (RawRow (RawNat4 10) (RawBool4 true) (RawStatus4 High4) (RawCount4 2))
    (RawRow (RawNat4 3) (RawBool4 true) (RawStatus4 Low4) (RawCount4 9))
    (RawRow (RawNat4 4) (RawBool4 true) (RawStatus4 Medium4) (RawCount4 5))
    (RawRow (RawNat4 8) (RawBool4 true) (RawStatus4 Low4) (RawCount4 1))
    (RawRow (RawNat4 2) (RawBool4 true) (RawStatus4 High4) (RawCount4 4))
    (RawRow (RawNat4 7) (RawBool4 false) (RawStatus4 Medium4) (RawCount4 3))
    (RawRow (RawNat4 6) (RawBool4 true) (RawStatus4 Medium4) (RawCount4 2))
    (RawRow (RawNat4 5) (RawBool4 true) (RawStatus4 Low4) (RawBool4 true)).

Example bad_array8_is_rejected : run_array8 bad_array8 = None.
Proof. reflexivity. Qed.
