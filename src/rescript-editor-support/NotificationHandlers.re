open Infix;
open RResult;
open TopTypes;
module J = JsonShort;

let getTextDocument = doc => {
  let%opt uri = Json.get("uri", doc) |?> Json.string;
  let%opt text = Json.get("text", doc) |?> Json.string;
  Some((uri, text));
};

let watchedFileContentsMap = Hashtbl.create(100);

let reloadAllState = state => {
  Log.log("RELOADING ALL STATE");
  {...TopTypes.empty(), documentText: state.documentText};
};

let notificationHandlers:
  list((string, (state, Json.t) => result(state, string))) = [
  (
    "textDocument/didOpen",
    (state, params) => {
      let%try (uri, text) =
        Json.get("textDocument", params)
        |?> getTextDocument
        |> RResult.orError("Invalid params");
      Hashtbl.replace(state.documentText, uri, text);

      let%try path = Utils.parseUri(uri) |> RResult.orError("Invalid uri");
      if (FindFiles.isSourceFile(path)) {
        let%try package = Packages.getPackage(uri, state);
        /* let name = FindFiles.getName(path); */
        if (!Hashtbl.mem(package.nameForPath, path)) {
          /* TODO: figure out what the name should be, and process it. */
          package.nameForPath
          |> Hashtbl.iter((name, _) => Log.log(" > " ++ name));
          Log.log("Reloading because you created a new file: " ++ path);
          Ok(state);
          /* Ok(reloadAllState(state)) */
          /* Hashtbl.add(package.nameForPath, path, name);
             Hashtbl.add(package.pathsForModule, name, Impl(path, Some(path)));
             Hashtbl.replace(state.packagesByRoot, package.basePath, {
               ...package,
               localModules: [name, ...package.localModules]
             });
             Ok(state) */
        } else {
          Ok(state);
        };
      } else {
        Ok(state);
      };
    },
  ),
  (
    "workspace/didChangeConfiguration",
    (state, _params) => {
      Ok(state);
    },
  ),
  (
    "textDocument/didSave",
    (state, _params) => {
      Ok(state);
    },
  ),
  (
    "textDocument/didChange",
    (state, params) => {
      open InfixResult;
      let%try doc = params |> RJson.get("textDocument");
      let%try uri = RJson.get("uri", doc) |?> RJson.string;
      let%try changes = RJson.get("contentChanges", params) |?> RJson.array;
      let%try text =
        List.nth(changes, List.length(changes) - 1)
        |> RJson.get("text")
        |?> RJson.string;
      /* Hmm how do I know if it's modified? */
      let state = State.updateContents(uri, text, state);
      Ok(state);
    },
  ),
  (
    "workspace/didChangeWatchedFiles",
    (state, params) => {
      Log.log("Got a watched file change");
      let%try changes = RJson.get("changes", params);
      let%try changes = RJson.array(changes);
      open InfixResult;
      let shouldReload =
        changes
        |> List.exists(change =>
             {
               let%try uri = RJson.get("uri", change) |?> RJson.string;
               let isRelevant = Utils.endsWith(uri, "/bsconfig.json");
               if (!isRelevant) {
                 Ok(false);
               } else {
                 let%try path =
                   Utils.parseUri(uri) |> RResult.orError("Cannot parse URI");
                 let%try contents = Files.readFileResult(path);
                 if (!Hashtbl.mem(watchedFileContentsMap, uri)
                     || Hashtbl.find(watchedFileContentsMap, uri) == contents) {
                   Ok(false);
                 } else {
                   Hashtbl.replace(watchedFileContentsMap, uri, contents);
                   Log.log("Reloading because a file changed: " ++ uri);
                   Ok(true);
                 };
               };
             }
             |? false
           );

      if (shouldReload) {
        Ok(reloadAllState(state));
      } else {
        Ok(state);
      };
    },
  ),
];
