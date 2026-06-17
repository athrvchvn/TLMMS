import axios from "axios";

namespace Requests {
  export function POST(endpoint: string, headers: Record<string, string>, body: Record<string, string>) {
    const url = import.meta.env.VITE_BACKEND_URL + endpoint;
    return axios.post(url, body, {
      headers,
    })
  }
}

export default Requests;
