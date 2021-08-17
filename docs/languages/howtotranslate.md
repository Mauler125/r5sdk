# **How to Translate**

### If you would like to translate it is very easy!
---
All you need to do is copy the `en-template` folder located in `docs/languages/` and rename it.<br/>
```
docs/languages/

├───en-template
│   ├───about
│   ├───faq
│   ├───installation
│   └───servers
├───fr
│   ├───about
│   ├───faq
│   ├───installation
│   └───servers
└───[Your language here]
    ├───about
    ├───faq
    ├───installation
    └───servers
```
You will now have a copy of the entire docs, feel free to edit anything inside your copy just don't change filenames as they are required to link pages together.<br/>
Also dont forget to include a link to your translation in `docs/index.md` at the bottom with the other languages
```
## Languages
 - [français](languages/fr)
 - Your language here
```

### Need further help?
reach out to me on discord `Switch#9867`

## Notes:

You will often see links like this:
```
[Are there bugs?](#are-there-bugs)
```
These links differ from other links because they don't link to another page or to another website.<br/>
These links, known as anchor links, link to content on the same page.<br/>
The method I use to create anchor links is shorthand provided by github. Instead of naming the header you can link to it directly by dropping special characters, replacing spaces with hyphens `-` and making it all lowercase.<br/>
For instance, the header that the above link links to looks like this:
```
## Are there bugs?
```
Converting between the two in english is easy because we exclusively use the Latin alphabet, however in other languages you have non-ascii characters.<br/>

According to user [googlielmo](https://gist.github.com/asabaylus/3071099#gistcomment-1798560) this shorthand will still work with non-ascii characters just be sure to follow the [rules](https://gist.github.com/asabaylus/3071099#anchors-in-markdown) outlined above.<br/>

Feel free to change any header text just note that these anchor links will no longer work without also changing the links themselves.<br/>

As with any link, you are able to change the text in between the `[ ]` as this text is displayed to the user. However, the text contained inside the `( )` must match the header it links to folowing the [rules](https://gist.github.com/asabaylus/3071099#anchors-in-markdown).