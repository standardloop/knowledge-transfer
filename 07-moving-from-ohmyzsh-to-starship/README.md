# Moving from oh-my-zsh to starhip

## Background

https://github.com/ohmyzsh/ohmyzsh

*Oh My Zsh is an open source, community-driven framework for managing your zsh configuration.*

https://github.com/starship/starship

*The minimal, blazing-fast, and infinitely customizable prompt for any shell!*

## Motivation

I've been using oh-my-zsh for the past 8 years when a coworker recommended it during an internship I had.

Recently I heard about starship and wanted to switch things up and see if it would be snappier.

## Steps

### Uninstalling and Cleanup

```sh
$ ~/.oh-my-zsh/tools/uninstall.sh
# This seems to create an automatic backup of the .zshrc
# location: ~/.zshrc.omz-uninstalled-****-**-**_**-**-**
$ rm -rf ~/.oh-my-zsh

$ code ~/.zshrc
# Now we need to cleanup all the oh-my-zsh config that was in the .zshrc
```


### Installing and Configuring Starship

I used brew for this

```sh
$ brew install starship
```

I've started with a simple preset

```sh
$ starship preset plain-text-symbols -o ~/.config/starsip.toml
```

Now I'll edit it slightly...
```sh
$ code ~/.config/starship.toml
# ...
```

Here are some sections that I have changed:

```toml
format = "$directory$git_branch$character"
add_newline = false

[character]
success_symbol = "[>](bold green)"
error_symbol = "[>](bold red)"
```

Looks like this now:

<pre>
<font color="#02ddff">knowledge-transfer</font><font color="#ffffff"> on</font> <font color="#cd79fe">git main</font> <font color="#00ff40">>  </font>
</pre>

#### Quick Tip

use `starship timings` to check your speed.

<pre>
<font color="#02ddff">knowledge-transfer</font><font color="#ffffff"> on</font> <font color="#cd79fe">git main</font> <font color="#00ff40">></font><font color="#ffffff"> starship timings</font>

 Here are the timings of modules in your prompt (>=1ms or output):
 directory   -  22ms  -   "<font color="#02ddff">knowledge-transfer</font> "
 character   -  <1ms  -   "<font color="#00ff40">> </font>"
 git_branch  -  <1ms  -   "on <font color="#cd79fe">git main</font> "
</pre>

### The One thing that was missing that I immediately noticed

The bind keys for moving the front and back of the prompt with the macOS function keys!

Add them by adding these lines to your `~/.zshrc` config:

```sh
bindkey "^[[H" beginning-of-line
bindkey "^[[F" end-of-line
```
