# Playtesting with AI

OpenToy looks at what the toy does, how it feels to play with, and how it handles failure.

- **Blue Team · Does it work?** Compare what the program is meant to do with
  what the physical toy actually does. An acknowledgment can confirm that a
  command reached the board. Seeing the response confirms what the player gets.
- **Green Team · Does it invite play?** Notice what people discover on their
  own, what they want to do again, and where they lose interest. The timing
  and feel of a working feature are part of its play value.
- **Red Team · What makes it fail?** Look for deliberate stress and unexpected
  everyday use that can interrupt a feature or stop the toy. Explain what the
  player experiences and how the toy recovers.

### The hidden design lesson in each chapter

| Chapter | What AI can help students notice |
| --- | --- |
| [Bare XIAO board](../projects/xiao-toy/README.md) | Even one LED gives the player feedback. The app can acknowledge a change to the blink rhythm, while the LED shows whether the change is visible and understandable. |
| [OLED + touch board](../projects/xiao-class/README.md) | A touch reading, a display update, and a reaction someone sees are different pieces of evidence. How the toy is held can affect whether a touch control feels deliberate or accidental. |
| [Face v1 · Eye puppetry](../docs/V1-EYES.md) | Pupils and eyelids can suggest attention or attitude with just a few shapes. The same eyes can look alive or awkward depending on how the controls map to gaze and winks. |
| [Face v2 · Doodle puppetry](../docs/V2-FACE.md) | Timing and combinations give simple curves personality. A blink, mouth pull, or springy settle can change how a pose reads even when the expression preset stays the same. |
| [Nearby Buddy](https://github.com/yury-g/OpenToy-Coursework/blob/feature/nearby-buddy/projects/nearby-buddy/README.md#how-to-playtest) | Bluetooth signal strength gives the toys an imperfect clue about distance. The timing of a smile, a pause, or a transition to sleep determines whether the pair seems to notice each other. |
| [Orange Face](https://github.com/yury-g/OpenToy-Coursework/blob/feature/orange-face/projects/orange-face/README.md#how-to-playtest) | A toy can sense continuous motion and wait for a clear gesture before reacting. The delay before rotation, the different tickle reactions, and whether the rear glow is visible all shape how the player reads the response. |
| [Plant v0.2](../mechanical/plant-v02/README.md) | Turn, nod, and roll can give a body a vocabulary of gestures. The CAD animation shows how that could look. The feel of those gestures depends on the assembled joints and powered motion, which are still unverified. |

### Give this to your AI assistant

Share this section and the chapter you’re looking at, or copy the prompt below.

```text
Read the OpenToy chapter I'm looking at and any bench notes, images,
or observations I already have. Help me see one design choice the toy
makes and why it matters in another electronic toy.

Use Blue Team for what the physical toy does, Green Team for what players
actually discover or enjoy, and Red Team for how a feature might fail under
deliberate stress or unexpected use and how it could recover. Say which
claims come from a demo, code, hardware checks, or player observations.
Treat untried failure cases as possibilities and leave play value open
when there are no player observations.

For setup and troubleshooting, run the checks you can access and verify
the fix. Ask for one specific action only when you need my access, input,
or hands on the hardware. Offer examples to notice during play. Keep
them optional and avoid assigning work.
```
