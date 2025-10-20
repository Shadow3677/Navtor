# Navtor pre-interview tasks

# Snow White and the Dwarfs Party

This document calculates the number of dwarfs wearing orange hats at Snow White's party.

## Variables

Let:  

- **G** = number of dwarfs in green hats  
- **O** = number of dwarfs in orange hats  
- **R** = number of dwarfs in red hats  

**Total dwarfs at the party:**  

```math
G + O + R = 43
```
## Questions
- "Are you wearing a green hat?"
- "Are you wearing an orange hat?"
- "Are you wearing a red hat?"

## Dwarfs' Responses

- Green: (yes, no, no)  
- Orange: (yes, yes, yes) or (no, no, no)  
- Red: (yes, yes, no)  

## Equations

1. `G + R + (O - x) = 34`  
2. `R + (O - x) = 26`  
3. `O - x = 11`  

## Solution

```text
From equation (3):
O - x = 11

Substitute into equation (2):
R + 11 = 26
R = 15

Substitute into equation (1):
G + 15 + 11 = 34
G = 8

Calculate O using the total number of Dwarfs:
G + O + R = 43
8 + O + 15 = 43
O = 20
```

| Hat Color | Number of Dwarfs |
| --------- | ---------------- |
| Green     | 8                |
| Orange    | 20               |
| Red       | 15               |
| **Total** | 43               |


##Conclusion
20 Dwarfs were wearing orange hats at the party.

## Back to the future
