import { Application, Router } from "https://deno.land/x/oak/mod.ts";
import testIDs from "./test_ids.json" assert { type: "json" };

const app = new Application();
const port = 8080;

const router = new Router();
router.get("/", ({ response }: { response: { body : { userIDs: number[] }}}) => {
  response.body = testIDs;
});
app.use(router.routes());
app.use(router.allowedMethods());

app.addEventListener("listen", ({ secure, hostname, port }) => {
  const protocol = secure ? "https://" : "http://";
  const url = `${protocol}${hostname ?? "localhost"}:${port}`;
  console.log(`Listening at ${url} on: ${port}`);
});

await app.listen({ port });